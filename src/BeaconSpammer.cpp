#include "BeaconSpammer.h"
#include "App.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include "Config.h"

// --- Beacon Packet Templates ---
// Based on your provided code, which is excellent.
static uint8_t open_beacon_template[] = {0x80, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x64, 0x00, 0x01, 0x04, 0x00};
static uint8_t open_beacon_post_ssid_tags[] = {0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, 0x03, 0x01, 0x01};

static uint8_t wpa2_beacon_template[] = {0x80, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x64, 0x00, 0x31, 0x04, 0x00};
static uint8_t wpa2_beacon_post_ssid_tags[] = {0x01, 0x08, 0x82, 0x84, 0x8b, 0x96, 0x24, 0x30, 0x48, 0x6c, 0x03, 0x01, 0x01, 0x30, 0x14, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x04, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x04, 0x01, 0x00, 0x00, 0x0f, 0xac, 0x02, 0x00, 0x00};

// Cycle through the best channels for maximum impact
const int BeaconSpammer::CHANNELS_TO_SPAM[] = {1, 6, 11, 2, 7, 12, 3, 8, 4, 9, 5, 10};

BeaconSpammer::BeaconSpammer() :
    app_(nullptr),
    isActive_(false),
    currentMode_(BeaconSsidMode::RANDOM),
    currentChannel_(1),
    ssidReader_(), // Default constructed
    ssidCounter_(0),
    lastPacketTime_(0),
    lastChannelHopTime_(0),
    channelHopIndex_(0)
{}

BeaconSpammer::~BeaconSpammer() {
    // The LineReader's destructor will automatically handle closing the file.
}

void BeaconSpammer::setup(App* app) {
    app_ = app;
}

bool BeaconSpammer::start(std::unique_ptr<HardwareManager::RfLock> rfLock, BeaconSsidMode mode, const std::string& ssidFilePath) {
    if (isActive_) return false;
    if (!rfLock || !rfLock->isValid()) {
        Serial.println("[BEACON] CRITICAL: Failed to acquire RF hardware lock. Aborting.");
        return false;
    }
    rfLock_ = std::move(rfLock);
    
    Serial.printf("[BEACON] Starting Beacon Spam. Mode: %d\n", (int)mode);
    currentMode_ = mode;
    
    if (currentMode_ == BeaconSsidMode::FILE_BASED) {
        ssidReader_ = SdCardManager::openLineReader(ssidFilePath.c_str());
        // --- VALIDATION LOGIC ---
        // Try to read one line. If it's empty, our new readLine() implementation
        // has determined the file has no valid content.
        if (!ssidReader_.isOpen() || ssidReader_.readLine().isEmpty()) {
            Serial.printf("[BEACON] SSID file is invalid or contains no valid SSIDs: %s\n", ssidFilePath.c_str());
            if (ssidReader_.isOpen()) ssidReader_.close(); // Clean up the reader.
            rfLock_.reset(); // Release the hardware lock since we are failing.
            return false;    // Signal the failure to the caller menu.
        }
        // The check was successful, but readLine() advanced the file pointer.
        // We must close and reopen the reader to ensure the attack starts from the first line.
        ssidReader_.close();
        ssidReader_ = SdCardManager::openLineReader(ssidFilePath.c_str());
    }

    ssidCounter_ = 0;
    channelHopIndex_ = 0;
    // THE FIX: Use the central channel list
    currentChannel_ = Channels::WIFI_2_4GHZ[0];
    esp_wifi_set_channel(currentChannel_, WIFI_SECOND_CHAN_NONE);
    lastPacketTime_ = 0;
    lastChannelHopTime_ = millis();
    isActive_ = true;
    
    Serial.printf("[BEACON] Attack started on Channel %d.\n", currentChannel_);
    return true;
}

void BeaconSpammer::stop() {
    if (!isActive_) return;
    Serial.println("[BEACON] Stopping beacon spam attack.");
    isActive_ = false;
    ssidReader_.close(); // Explicitly close the reader
    rfLock_.reset();
    Serial.println("[BEACON] Attack stopped and RF lock released.");
}

void BeaconSpammer::loop() {
    if (!isActive_) return;
    
    unsigned long currentTime = millis();

    // --- PERFORMANCE FIX: Send packets as fast as possible ---
    // The original code had a 10ms delay. We can make this much smaller or 0.
    // A small delay prevents the watchdog timer from tripping on very fast ESP32s.
    const uint32_t PACKET_INTERVAL_US = 2000; // 2ms delay for stability
    if ((long)(micros() - lastPacketTime_) > PACKET_INTERVAL_US) {
        FakeAP ap = getNextFakeAP();
        if(!ap.ssid.empty()) { // Only send if we got a valid AP
            sendBeaconPacket(ap);
            ssidCounter_++;
        }
        lastPacketTime_ = micros();
    }

    const uint32_t CHANNEL_HOP_INTERVAL_MS = 2500;
    if (millis() - lastChannelHopTime_ > CHANNEL_HOP_INTERVAL_MS) {
        // THE FIX: Use the central channel list and count
        channelHopIndex_ = (channelHopIndex_ + 1) % Channels::WIFI_2_4GHZ_COUNT;
        currentChannel_ = Channels::WIFI_2_4GHZ[channelHopIndex_];
        esp_wifi_set_channel(currentChannel_, WIFI_SECOND_CHAN_NONE);
        lastChannelHopTime_ = millis();
    }
}

// --- The "Generator" function ---
BeaconSpammer::FakeAP BeaconSpammer::getNextFakeAP() {
    FakeAP ap;
    ap.is_wpa2 = random(0, 2) == 0;
    for (int j = 0; j < 6; j++) ap.bssid[j] = esp_random() & 0xFF;
    ap.bssid[0] = (ap.bssid[0] & 0xFE) | 0x02;

    if (currentMode_ == BeaconSsidMode::FILE_BASED && ssidReader_.isOpen()) {
        String ssid = ssidReader_.readLine(); // This is now robust
        if (ssid.length() > 32) ssid = ssid.substring(0, 32);
        ap.ssid = ssid.c_str(); // Will be empty if file ends, handled in loop()
    } else { // RANDOM mode
        char random_buffer[33];
        const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        int ssid_len = 8 + (esp_random() % 17);
        for (int i = 0; i < ssid_len; ++i) {
            random_buffer[i] = charset[esp_random() % (sizeof(charset) - 2)];
        }
        random_buffer[ssid_len] = '\0';
        ap.ssid = random_buffer;
    }
    return ap;
}

void BeaconSpammer::sendBeaconPacket(const FakeAP& ap) {
    uint8_t packet[256];
    uint8_t ssidLen = ap.ssid.length();
    // This check is already done in getNextFakeAP, but good for safety
    if (ssidLen > 32) ssidLen = 32;

    const uint8_t* template_data;
    const uint8_t* post_ssid_tags_data;
    size_t template_len;
    size_t post_tags_len;

    if (ap.is_wpa2) {
        template_data = RawFrames::Mgmt::Beacon::WPA2_TEMPLATE;
        template_len = sizeof(RawFrames::Mgmt::Beacon::WPA2_TEMPLATE);
        post_ssid_tags_data = RawFrames::Mgmt::Beacon::WPA2_POST_SSID_TAGS;
        post_tags_len = sizeof(RawFrames::Mgmt::Beacon::WPA2_POST_SSID_TAGS);
    } else {
        template_data = RawFrames::Mgmt::Beacon::OPEN_TEMPLATE;
        template_len = sizeof(RawFrames::Mgmt::Beacon::OPEN_TEMPLATE);
        post_ssid_tags_data = RawFrames::Mgmt::Beacon::OPEN_POST_SSID_TAGS;
        post_tags_len = sizeof(RawFrames::Mgmt::Beacon::OPEN_POST_SSID_TAGS);
    }
    
    // The actual packet starts with the template, but SSID length is at template_len, not template_len-1.
    // The byte at template_len is the tag number (0x00 for SSID) and template_len+1 is the length. Let's fix this.
    size_t total_len = (template_len - 1) + 1 + 1 + ssidLen + post_tags_len; // template (minus tag) + tag_num + tag_len + ssid + post_tags
    if (total_len > sizeof(packet)) return;

    memcpy(packet, template_data, template_len - 1); // Copy up to the SSID tag number
    uint8_t* p = packet + template_len - 1;

    *p++ = 0x00; // SSID Tag Number
    *p++ = ssidLen; // SSID Tag Length
    memcpy(p, ap.ssid.c_str(), ssidLen);
    p += ssidLen;

    memcpy(p, post_ssid_tags_data, post_tags_len);
    
    // Overwrite BSSID and Source MAC in the final packet
    memcpy(&packet[10], ap.bssid, 6);
    memcpy(&packet[16], ap.bssid, 6);

    // Overwrite channel in DS Parameter Set Tag
    uint8_t* ds_param_set_tag = packet + total_len - post_tags_len + post_tags_len - 3;
    ds_param_set_tag[2] = currentChannel_;

    esp_wifi_80211_tx(WIFI_IF_STA, packet, total_len, false);
}

bool BeaconSpammer::isSsidFileValid(const std::string& ssidFilePath) {
    // Use our robust line reader to perform the check.
    auto reader = SdCardManager::openLineReader(ssidFilePath.c_str());
    if (!reader.isOpen()) {
        return false;
    }
    // readLine() now correctly returns an empty string if the file is empty or only has whitespace.
    // The reader's destructor will close the file automatically when it goes out of scope.
    return !reader.readLine().isEmpty();
}


bool BeaconSpammer::isActive() const { return isActive_; }
uint32_t BeaconSpammer::getSsidCounter() const { return ssidCounter_; }
int BeaconSpammer::getCurrentChannel() const { return currentChannel_; }