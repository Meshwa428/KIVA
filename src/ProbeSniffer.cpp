#include "ProbeSniffer.h"
#include "App.h" // For logging and SD manager access
#include "Logger.h"
#include "SdCardManager.h"

// Channel hopping configuration
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

    pcapFile_ = SdCardManager::getInstance().openFileUncached(currentPcapFilename_, FILE_WRITE);
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
    
    // Clear the session SSID list file for this new session
    SdCardManager::getInstance().deleteFile(SD_ROOT::DATA_PROBES_SSID_SESSION);

    LOG(LogLevel::INFO, "PROBE", "PCAP file created: %s", currentPcapFilename_);
}

void ProbeSniffer::closePcapFile() {
    if (pcapFile_) {
        pcapFile_.close();
        LOG(LogLevel::INFO, "PROBE", "PCAP file closed. Captured %u probe packets.", packetCount_);
        // --- REMOVED THE POPUP ---
    }
}

bool ProbeSniffer::start() {
    if (isActive_) return true;
    
    // Request hardware lock from the manager
    rfLock_ = app_->getHardwareManager().requestRfControl(RfClient::WIFI_PROMISCUOUS);
    if (!rfLock_ || !rfLock_->isValid()) {
        LOG(LogLevel::ERROR, "PROBE", "Failed to acquire RF Lock for sniffing.");
        return false;
    }

    openPcapFile();
    if (!pcapFile_) {
        rfLock_.reset(); // Release lock if file creation fails
        return false;
    }

    LOG(LogLevel::INFO, "PROBE", "Starting probe request sniffing to PCAP.");
    packetCount_ = 0;
    uniqueSsids_.clear();
    channelHopIndex_ = 0;
    
    // The hardware is already in the correct mode. We just set the callback and channel.
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

    // The RfLock destructor will automatically handle turning off promiscuous mode
    // and shutting down the WiFi radio.
    rfLock_.reset();
    
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

    // Check for Management Frame (type 0) with Probe Request subtype (4)
    if ((frame[0] != 0x40)) {
        return; // Not a Probe Request, so we ignore it.
    }
    
    // It's a Probe Request, now parse it.
    uint8_t ssid_length = frame[25];

    if (ssid_length > 0 && ssid_length <= 32) {
        char ssid[33] = {0};
        memcpy(ssid, &frame[26], ssid_length);

        // --- Write to PCAP ---
        packetCount_++;
        uint32_t timestamp_sec = packet->rx_ctrl.timestamp / 1000000;
        uint32_t timestamp_usec = packet->rx_ctrl.timestamp % 1000000;
        uint32_t captured_len = packet->rx_ctrl.sig_len;

        pcapFile_.write((uint8_t*)&timestamp_sec, 4);
        pcapFile_.write((uint8_t*)&timestamp_usec, 4);
        pcapFile_.write((uint8_t*)&captured_len, 4);
        pcapFile_.write((uint8_t*)&captured_len, 4); // Original length is same as captured
        pcapFile_.write(packet->payload, captured_len);

        if (packetCount_ % 10 == 0) {
            pcapFile_.flush();
        }

        // --- Update UI List & Save to Files (de-duplicated) ---
        bool exists = false;
        for (const auto& existingSsid : uniqueSsids_) {
            if (existingSsid == ssid) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            uniqueSsids_.push_back(ssid);

            // 1. Write to the session file
            File sessionFile = SdCardManager::getInstance().openFileUncached(SD_ROOT::DATA_PROBES_SSID_SESSION, FILE_APPEND);
            if (sessionFile) {
                sessionFile.println(ssid);
                sessionFile.close();
            }

            // 2. Check cumulative file and append if not present
            bool alreadyInCumulative = false;
            auto reader = SdCardManager::getInstance().openLineReader(SD_ROOT::DATA_PROBES_SSID_CUMULATIVE);
            if(reader.isOpen()) {
                while(true) {
                    String line = reader.readLine();
                    if (line.isEmpty()) break;
                    if (line == ssid) {
                        alreadyInCumulative = true;
                        break;
                    }
                }
                reader.close();
            }

            if (!alreadyInCumulative) {
                File cumulativeFile = SdCardManager::getInstance().openFileUncached(SD_ROOT::DATA_PROBES_SSID_CUMULATIVE, FILE_APPEND);
                if (cumulativeFile) {
                    cumulativeFile.println(ssid);
                    cumulativeFile.close();
                }
            }
        }
    }
}

// --- Getters ---
bool ProbeSniffer::isActive() const { return isActive_; }
uint32_t ProbeSniffer::getPacketCount() const { return packetCount_; }
const std::vector<std::string>& ProbeSniffer::getUniqueSsids() const { return uniqueSsids_; }
uint32_t ProbeSniffer::getResourceRequirements() const {
    return isActive_ ? (uint32_t)ResourceRequirement::WIFI : (uint32_t)ResourceRequirement::NONE;
}