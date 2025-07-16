#include "Jammer.h"
#include "App.h"

// --- Pre-defined channel lists ---
const int Jammer::ble_adv_nrf_channels_[] = {2, 26, 80};
const int Jammer::bt_classic_nrf_channels_[] = {
    2,  5,  8, 11, 14, 17, 20, 23, 26, 29, 32, 35, 38, 41,
    44, 47, 50, 53, 56, 59, 62, 65, 68, 71, 74, 77, 80
};
// This list now represents the TARGET WI-FI CHANNELS to sweep.
const int Jammer::wifi_narrow_nrf_channels_[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};

// Garbage payload for Noise Injection
static const uint8_t garbage_payload_[] = { 
    0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55,
    0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55
};


Jammer::Jammer() :
    app_(nullptr),
    isActive_(false),
    currentMode_(JammingMode::IDLE),
    channelHopIndex_(0)
{
}

void Jammer::setup(App* app) {
    app_ = app;
    Serial.println("[JAMMER-LOG] Jammer Manager initialized.");
}

bool Jammer::start(std::unique_ptr<HardwareManager::RfLock> rfLock, JammingMode mode, JammerConfig config) {
    if (isActive_) {
        Serial.println("[JAMMER-LOG] Jammer already active. Please stop first.");
        return false;
    }

    if (!rfLock || !rfLock->isValid()) {
        Serial.println("[JAMMER-LOG] CRITICAL: Failed to acquire RF hardware lock. Aborting.");
        return false;
    }
    
    rfLock_ = std::move(rfLock);
    Serial.printf("[JAMMER-LOG] Attempting to start JammingMode %d\n", (int)mode);
    
    isActive_ = true;
    currentMode_ = mode;
    currentConfig_ = config;
    channelHopIndex_ = 0;
    
    byte dummy_addr[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
    if (rfLock_->radio1) rfLock_->radio1->openWritingPipe(dummy_addr);
    if (rfLock_->radio2) rfLock_->radio2->openWritingPipe(dummy_addr);
    
    int initialChannel = 1;

    switch (mode) {
        case JammingMode::BLE:
        case JammingMode::BT_CLASSIC:
            initialChannel = (mode == JammingMode::BLE) ? ble_adv_nrf_channels_[0] : bt_classic_nrf_channels_[0];
            if (rfLock_->radio1) rfLock_->radio1->startConstCarrier(RF24_PA_MAX, initialChannel);
            if (rfLock_->radio2) rfLock_->radio2->startConstCarrier(RF24_PA_MAX, initialChannel);
            break;
        case JammingMode::WIFI_NARROWBAND:
        case JammingMode::WIDE_SPECTRUM:
        case JammingMode::CHANNEL_FLOOD_CUSTOM:
            break;
        default: break;
    }

    Serial.printf("[JAMMER-LOG] Jammer started. Mode: %s\n", getModeString());
    return true;
}

void Jammer::stop() {
    if (!isActive_) {
        return;
    }
    Serial.println("[JAMMER-LOG] Stopping active jamming...");
    
    if (rfLock_ && rfLock_->radio1) rfLock_->radio1->stopListening();
    if (rfLock_ && rfLock_->radio2) rfLock_->radio2->stopListening();

    rfLock_.reset();
    
    isActive_ = false;
    currentMode_ = JammingMode::IDLE;
    Serial.println("[JAMMER-LOG] Jammer stopped and RF lock released.");
}

void Jammer::loop() {
    if (!isActive_ || !rfLock_) {
        return;
    }

    switch (currentMode_) {
        case JammingMode::BLE: {
            int numChannels = sizeof(ble_adv_nrf_channels_) / sizeof(int);
            channelHopIndex_ = (channelHopIndex_ + 1) % numChannels;
            int nextChannel = ble_adv_nrf_channels_[channelHopIndex_];
            if (rfLock_->radio1) rfLock_->radio1->startConstCarrier(RF24_PA_MAX, nextChannel);
            if (rfLock_->radio2) rfLock_->radio2->startConstCarrier(RF24_PA_MAX, nextChannel);
            break;
        }
        case JammingMode::BT_CLASSIC: {
            int numChannels = sizeof(bt_classic_nrf_channels_) / sizeof(int);
            channelHopIndex_ = (channelHopIndex_ + 1) % numChannels;
            int nextChannel = bt_classic_nrf_channels_[channelHopIndex_];
            if (rfLock_->radio1) rfLock_->radio1->startConstCarrier(RF24_PA_MAX, nextChannel);
            if (rfLock_->radio2) rfLock_->radio2->startConstCarrier(RF24_PA_MAX, nextChannel);
            break;
        }
        case JammingMode::WIFI_NARROWBAND:
        case JammingMode::CHANNEL_FLOOD_CUSTOM: {
            int target_wifi_channel;
            int num_wifi_channels;

            if (currentMode_ == JammingMode::WIFI_NARROWBAND) {
                num_wifi_channels = sizeof(wifi_narrow_nrf_channels_) / sizeof(int);
                if (num_wifi_channels == 0) return;
                target_wifi_channel = wifi_narrow_nrf_channels_[channelHopIndex_];
            } else { // CHANNEL_FLOOD_CUSTOM
                num_wifi_channels = currentConfig_.customChannels.size();
                if (num_wifi_channels == 0) return;
                target_wifi_channel = currentConfig_.customChannels[channelHopIndex_];
            }
            
            // --- BARRAGE JAMMING / SWEEP LOGIC ---
            // Calculate the NRF channel range to sweep for the target Wi-Fi channel.
            int start_nrf_channel = (target_wifi_channel * 5) + 1;
            int end_nrf_channel = start_nrf_channel + 22; // Sweep across 22 MHz

            for (int i = start_nrf_channel; i < end_nrf_channel; ++i) {
                if (i > 125) continue; // Stay within valid NRF channel range
                jamWithNoise(i, i); // Jam this 1MHz slice with both radios
            }
            
            // Move to the next target Wi-Fi channel for the next loop() iteration
            channelHopIndex_ = (channelHopIndex_ + 1) % num_wifi_channels;
            break;
        }
        case JammingMode::WIDE_SPECTRUM: {
            int nextChannel1 = random(0, 126);
            int nextChannel2 = random(0, 126);
            jamWithNoise(nextChannel1, nextChannel2);
            break;
        }
        default:
            return;
    }
}

void Jammer::jamWithNoise(int channel1, int channel2) {
    if (rfLock_->radio1 && channel1 != -1) {
        rfLock_->radio1->setChannel(channel1);
        rfLock_->radio1->write(&garbage_payload_, sizeof(garbage_payload_));
    }

    delayMicroseconds(50); // Crucial delay for SPI bus stability

    if (rfLock_->radio2 && channel2 != -1) {
        rfLock_->radio2->setChannel(channel2);
        rfLock_->radio2->write(&garbage_payload_, sizeof(garbage_payload_));
    }
}


// --- State Getters ---

bool Jammer::isActive() const {
    return isActive_;
}

JammingMode Jammer::getCurrentMode() const {
    return currentMode_;
}

const char* Jammer::getModeString() const {
    switch(currentMode_) {
        case JammingMode::BLE: return "BLE Jam";
        case JammingMode::BT_CLASSIC: return "BT Classic Jam";
        case JammingMode::WIFI_NARROWBAND: return "WiFi Barrage";
        case JammingMode::WIDE_SPECTRUM: return "Wide Spectrum Barrage";
        case JammingMode::CHANNEL_FLOOD_CUSTOM: return "Custom Barrage";
        default: return "Idle";
    }
}