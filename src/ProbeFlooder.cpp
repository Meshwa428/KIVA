#include "ProbeFlooder.h"
#include "App.h"
#include "Logger.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include "Config.h"

// Base 802.11 Probe Request frame
static uint8_t prob_req_packet_template[24] = {
    0x40, 0x00, 0x00, 0x00, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Destination (Broadcast)
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // Source (Randomized later)
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // BSSID (Broadcast)
    // Sequence number (bytes 22, 23) is set to 0 and managed by hardware
};

// Standard tagged parameters that follow the SSID in a probe request
static uint8_t post_ssid_tags[12] = {
    0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, // Tag 1: Supported Rates
    0x32, 0x04, // Tag 50: Extended Supported Rates
};

// Channels to hop through for maximum coverage
const int ProbeFlooder::CHANNELS_TO_SPAM[] = {1, 6, 11, 2, 7, 12, 3, 8, 4, 9, 5, 10};

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
    if (isActive_) return false;
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
    // THE FIX: Use the central channel list
    currentChannel_ = Channels::WIFI_2_4GHZ[0];
    esp_wifi_set_channel(currentChannel_, WIFI_SECOND_CHAN_NONE);
    lastChannelHopTime_ = millis();
    isActive_ = true;
    
    LOG(LogLevel::INFO, "PROBE_FLOOD", "Attack started on Channel %d.", currentChannel_);
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
    
    // Send a burst of packets on each loop iteration for high throughput
    for (int i = 0; i < 20; ++i) {
        std::string ssid = getNextSsid();
        if (!ssid.empty()) {
            sendProbePacket(ssid);
            packetCounter_++;
        } else if (currentMode_ == ProbeFloodMode::FILE_BASED) {
            // End of file, restart from beginning
            ssidReader_.close();
            ssidReader_ = SdCardManager::openLineReader(SD_ROOT::DATA_PROBES_SSID_SESSION);
        }
    }

    // Handle channel hopping
    const uint32_t CHANNEL_HOP_INTERVAL_MS = 250;
    if (millis() - lastChannelHopTime_ > CHANNEL_HOP_INTERVAL_MS) {
        // THE FIX: Use the central channel list and count
        channelHopIndex_ = (channelHopIndex_ + 1) % Channels::WIFI_2_4GHZ_COUNT;
        currentChannel_ = Channels::WIFI_2_4GHZ[channelHopIndex_];
        esp_wifi_set_channel(currentChannel_, WIFI_SECOND_CHAN_NONE);
        lastChannelHopTime_ = millis();
    }
}

std::string ProbeFlooder::getNextSsid() {
    if (currentMode_ == ProbeFloodMode::FILE_BASED && ssidReader_.isOpen()) {
        return ssidReader_.readLine().c_str();
    } else { // RANDOM mode
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
    memcpy(packet_buffer, prob_req_packet_template, sizeof(prob_req_packet_template));

    // Set random source MAC address
    for (int i = 0; i < 6; i++) {
        packet_buffer[10 + i] = esp_random() & 0xFF;
    }
    // Ensure it's a locally administered, unicast address
    packet_buffer[10] = (packet_buffer[10] & 0xFE) | 0x02;

    uint8_t ssid_len = std::min((int)ssid.length(), 32);
    
    // Construct tagged parameters section
    uint8_t* p = packet_buffer + sizeof(prob_req_packet_template);
    *p++ = 0; // Tag: SSID
    *p++ = ssid_len;
    memcpy(p, ssid.c_str(), ssid_len);
    p += ssid_len;
    
    memcpy(p, post_ssid_tags, sizeof(post_ssid_tags));
    p += sizeof(post_ssid_tags);

    int total_len = p - packet_buffer;
    
    // Use WIFI_IF_STA for raw packet injection when in promiscuous station mode.
    esp_wifi_80211_tx(WIFI_IF_STA, packet_buffer, total_len, false);
}

bool ProbeFlooder::isActive() const { return isActive_; }
uint32_t ProbeFlooder::getPacketCounter() const { return packetCounter_; }
int ProbeFlooder::getCurrentChannel() const { return currentChannel_; }