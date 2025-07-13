#include "Jammer.h"
#include "App.h" // For access to WifiManager
// #include <SPI.h> // For SPI communication with NRF24

// --- Pre-defined channel lists ---
// These are the NRF24L01 channels (0-125) that correspond to the target frequencies.
const int Jammer::ble_adv_nrf_channels_[] = {2, 26, 80};
const int Jammer::bt_classic_nrf_channels_[] = {
    2,  5,  8, 11, 14, 17, 20, 23, 26, 29, 32, 35, 38, 41,
    44, 47, 50, 53, 56, 59, 62, 65, 68, 71, 74, 77, 80
};
const int Jammer::wifi_narrow_nrf_channels_[] = {
    12, 17, 22, 27, 32, 37, 42, 47, 52, 57, 62, 67, 72
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

    // Check if the lock we received is valid
    if (!rfLock || !rfLock->isValid()) {
        Serial.println("[JAMMER-LOG] CRITICAL: Failed to acquire RF hardware lock. Aborting.");
        return false;
    }
    
    // The lock is valid, take ownership of it.
    rfLock_ = std::move(rfLock);

    Serial.printf("[JAMMER-LOG] Attempting to start JammingMode %d\n", (int)mode);
    
    // Set initial state
    isActive_ = true;
    currentMode_ = mode;
    currentConfig_ = config;
    channelHopIndex_ = 0;
    
    int initialChannel = 1; // Default starting channel
    switch (mode) {
        case JammingMode::BLE:
            initialChannel = ble_adv_nrf_channels_[0];
            break;
        case JammingMode::BT_CLASSIC:
            initialChannel = bt_classic_nrf_channels_[0];
            break;
        case JammingMode::WIFI_NARROWBAND:
            initialChannel = wifi_narrow_nrf_channels_[0];
            break;
        case JammingMode::WIDE_SPECTRUM:
            initialChannel = 0;
            break;
        case JammingMode::CHANNEL_FLOOD_CUSTOM:
            if (!config.customChannels.empty()) {
                initialChannel = config.customChannels[0];
            }
            break;
        default: break;
    }

    // Start constant carrier wave on the initial channel
    if (rfLock_->radio1) rfLock_->radio1->startConstCarrier(RF24_PA_MAX, initialChannel);
    if (rfLock_->radio2) rfLock_->radio2->startConstCarrier(RF24_PA_MAX, initialChannel);

    Serial.printf("[JAMMER-LOG] Jammer started. Initial channel: %d\n", initialChannel);
    return true;
}

void Jammer::stop() {
    if (!isActive_) {
        return;
    }
    Serial.println("[JAMMER-LOG] Stopping active jamming...");
    
    // This is the RAII magic. When rfLock_ is reset, its destructor is called,
    // which tells HardwareManager to power down the NRFs and release the lock.
    rfLock_.reset();
    
    isActive_ = false;
    currentMode_ = JammingMode::IDLE;
    Serial.println("[JAMMER-LOG] Jammer stopped and RF lock released.");
}

void Jammer::loop() {
    if (!isActive_ || !rfLock_) {
        return;
    }
    
    int nextChannel1 = -1;
    int nextChannel2 = -1;

    switch (currentMode_) {
        case JammingMode::BLE: {
            int numChannels = sizeof(ble_adv_nrf_channels_) / sizeof(int);
            channelHopIndex_ = (channelHopIndex_ + 1) % numChannels;
            nextChannel1 = ble_adv_nrf_channels_[channelHopIndex_];
            nextChannel2 = nextChannel1; // Both radios on the same channel
            break;
        }
        case JammingMode::BT_CLASSIC: {
            int numChannels = sizeof(bt_classic_nrf_channels_) / sizeof(int);
            channelHopIndex_ = (channelHopIndex_ + 1) % numChannels;
            nextChannel1 = bt_classic_nrf_channels_[channelHopIndex_];
            nextChannel2 = nextChannel1;
            break;
        }
        case JammingMode::WIFI_NARROWBAND: {
            int numChannels = sizeof(wifi_narrow_nrf_channels_) / sizeof(int);
            channelHopIndex_ = (channelHopIndex_ + 1) % numChannels;
            nextChannel1 = wifi_narrow_nrf_channels_[channelHopIndex_];
            nextChannel2 = nextChannel1;
            break;
        }
        case JammingMode::WIDE_SPECTRUM: {
            // Each radio jumps to a different random channel for max coverage
            nextChannel1 = random(0, 126);
            nextChannel2 = random(0, 126);
            break;
        }
        case JammingMode::CHANNEL_FLOOD_CUSTOM: {
            if (!currentConfig_.customChannels.empty()) {
                int numChannels = currentConfig_.customChannels.size();
                channelHopIndex_ = (channelHopIndex_ + 1) % numChannels;
                nextChannel1 = currentConfig_.customChannels[channelHopIndex_];
                nextChannel2 = nextChannel1;
            }
            break;
        }
        default:
            return; // Should not happen
    }

    if (nextChannel1 != -1 && rfLock_->radio1) {
        rfLock_->radio1->startConstCarrier(RF24_PA_MAX, nextChannel1);
    }
    if (nextChannel2 != -1 && rfLock_->radio2) {
        rfLock_->radio2->startConstCarrier(RF24_PA_MAX, nextChannel2);
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
        case JammingMode::WIFI_NARROWBAND: return "WiFi Narrowband";
        case JammingMode::WIDE_SPECTRUM: return "Wide Spectrum";
        case JammingMode::CHANNEL_FLOOD_CUSTOM: return "Custom Flood";
        default: return "Idle";
    }
}