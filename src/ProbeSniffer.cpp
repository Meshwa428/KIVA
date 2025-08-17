#include "ProbeSniffer.h"
#include "App.h" // For logging and SD manager access
#include "Logger.h"
#include "SdCardManager.h"

// Channel hopping configuration, as used in the reference project
static const int CHANNELS_TO_SNIFF[] = {1, 6, 11, 2, 7, 3, 8, 4, 9, 5, 10, 12, 13};
static const unsigned long CHANNEL_HOP_INTERVAL_MS = 333;

// Initialize the static instance pointer
ProbeSniffer* ProbeSniffer::instance_ = nullptr;

ProbeSniffer::ProbeSniffer() :
    app_(nullptr),
    isActive_(false),
    packetCount_(0),
    channelHopIndex_(0),
    lastChannelHopTime_(0)
{
    instance_ = this;
}

void ProbeSniffer::setup(App* app) {
    app_ = app;
}

void ProbeSniffer::openPcapFile() {
    snprintf(currentPcapFilename_, sizeof(currentPcapFilename_), "%s/probes_%lu.pcap", SD_ROOT::DATA_PROBES, millis());

    pcapFile_ = SdCardManager::openFile(currentPcapFilename_, FILE_WRITE);
    if (!pcapFile_) {
        LOG(LogLevel::ERROR, "PROBE", "Failed to create PCAP file: %s", currentPcapFilename_);
        return;
    }

    // Write the PCAP Global Header (24 bytes) for 802.11 frames
    uint8_t header[] = {
        0xd4, 0xc3, 0xb2, 0xa1, // Magic Number (little-endian)
        0x02, 0x00, 0x04, 0x00, // Version major, version minor
        0x00, 0x00, 0x00, 0x00, // Timezone offset
        0x00, 0x00, 0x00, 0x00, // Accuracy of timestamps
        0xff, 0xff, 0x00, 0x00, // Snapshot length
        0x69, 0x00, 0x00, 0x00  // Link-layer header type (105 for 802.11)
    };
    pcapFile_.write(header, sizeof(header));
    LOG(LogLevel::INFO, "PROBE", "PCAP file created: %s", currentPcapFilename_);
}

void ProbeSniffer::closePcapFile() {
    if (pcapFile_) {
        pcapFile_.close();
        LOG(LogLevel::INFO, "PROBE", "PCAP file closed. Captured %u probe packets.", packetCount_);
        if (app_) {
            char msg[100];
            snprintf(msg, sizeof(msg), "Saved %u probes to %s", packetCount_, currentPcapFilename_);
            app_->showPopUp("Capture Complete", msg, nullptr, "OK", "", true);
        }
    }
}

bool ProbeSniffer::start() {
    if (isActive_) return true;
    
    openPcapFile();
    if (!pcapFile_) {
        return false; // Failed to create file
    }

    LOG(LogLevel::INFO, "PROBE", "Starting probe request sniffing to PCAP.");
    packetCount_ = 0;
    uniqueSsids_.clear();
    channelHopIndex_ = 0;
    
    // --- Initialization inspired by Evil-Cardputer ---
    // This full re-initialization ensures a clean state for promiscuous mode.
    esp_wifi_set_promiscuous(false);
    esp_wifi_stop();
    delay(100);
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    // --- START OF FIX: Explicitly set Station mode before starting ---
    // This prevents the ESP32 from defaulting to AP mode and broadcasting an SSID.
    esp_wifi_set_mode(WIFI_MODE_STA);
    // --- END OF FIX ---
    
    esp_wifi_start();
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&packetHandlerCallback);
    esp_wifi_set_channel(CHANNELS_TO_SNIFF[0], WIFI_SECOND_CHAN_NONE);

    isActive_ = true;
    lastChannelHopTime_ = millis();
    return true;
}

void ProbeSniffer::stop() {
    if (!isActive_) return;
    LOG(LogLevel::INFO, "PROBE", "Stopping sniffing.");
    isActive_ = false;
    esp_wifi_set_promiscuous(false);
    closePcapFile();
}

void ProbeSniffer::loop() {
    if (!isActive_) return;

    // Handle channel hopping
    if (millis() - lastChannelHopTime_ > CHANNEL_HOP_INTERVAL_MS) {
        lastChannelHopTime_ = millis();
        channelHopIndex_ = (channelHopIndex_ + 1) % (sizeof(CHANNELS_TO_SNIFF) / sizeof(int));
        esp_wifi_set_channel(CHANNELS_TO_SNIFF[channelHopIndex_], WIFI_SECOND_CHAN_NONE);
    }
}

// Static callback that redirects to the instance method
void ProbeSniffer::packetHandlerCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (instance_ != nullptr && type == WIFI_PKT_MGMT) {
        instance_->handlePacket(static_cast<wifi_promiscuous_pkt_t*>(buf));
    }
}

void ProbeSniffer::handlePacket(wifi_promiscuous_pkt_t *packet) {
    const uint8_t *frame = packet->payload;

    // Frame Control field is at byte 0. We're looking for Management Frame (type 0)
    // with Probe Request subtype (4). The frame control field is the first two bytes.
    // frame[0] contains type and subtype.
    // Type is bits 2-3. Subtype is bits 4-7.
    // (frame[0] & 0b00001100) == 0b00000000 checks for Management Frame.
    // (frame[0] & 0b11110000) == 0b01000000 checks for Probe Request.
    if ((frame[0] != 0x40)) {
        return; // Not a Probe Request, so we ignore it.
    }
    
    // It's a Probe Request, now parse it.
    // The tagged parameters start at byte 24. Tag 0 is the SSID.
    uint8_t ssid_length = frame[25];

    // We only care about probes for specific (non-broadcast) SSIDs
    if (ssid_length > 0 && ssid_length <= 32) {
        char ssid[33] = {0};
        memcpy(ssid, &frame[26], ssid_length);

        // --- Write to PCAP ---
        packetCount_++;

        uint32_t timestamp_sec = packet->rx_ctrl.timestamp / 1000000;
        uint32_t timestamp_usec = packet->rx_ctrl.timestamp % 1000000;
        uint32_t captured_len = packet->rx_ctrl.sig_len - 4;

        pcapFile_.write((uint8_t*)&timestamp_sec, 4);
        pcapFile_.write((uint8_t*)&timestamp_usec, 4);
        pcapFile_.write((uint8_t*)&captured_len, 4);
        pcapFile_.write((uint8_t*)&captured_len, 4); // Original length is same as captured
        pcapFile_.write(packet->payload, captured_len);

        if (packetCount_ % 10 == 0) {
            pcapFile_.flush();
        }

        // --- Update UI List (de-duplicated) ---
        bool exists = false;
        for (const auto& existingSsid : uniqueSsids_) {
            if (existingSsid == ssid) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            uniqueSsids_.push_back(ssid);
        }
    }
}

// --- Getters ---
bool ProbeSniffer::isActive() const { return isActive_; }
uint32_t ProbeSniffer::getPacketCount() const { return packetCount_; }
const std::vector<std::string>& ProbeSniffer::getUniqueSsids() const { return uniqueSsids_; }