#include "Jammer.h"
#include "App.h"

// Pre-defined channel lists (no longer need reversed versions)
const int Jammer::ble_adv_nrf_channels_[] = {2, 26, 80};
const int Jammer::bt_classic_nrf_channels_[] = {
    0, 1, 2, 4, 6, 8, 22, 24, 26, 28, 30, 32, 34, 46, 48, 50, 52, 74, 76, 78, 80
};
const int Jammer::wifi_narrow_nrf_channels_[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};

// Garbage payload for Noise Injection
static const char jam_text[] = "xxxxxxxxxxxxxxxx";

Jammer::Jammer() :
    app_(nullptr),
    isActive_(false),
    currentMode_(JammingMode::IDLE),
    channelHopIndex_(0),
    lastHopTime_(0),
    sweepIndex_(0),
    targetWifiChannel_(0)
{}

void Jammer::setup(App* app) {
    app_ = app;
    Serial.println("[JAMMER-LOG] Jammer Manager initialized.");
}

bool Jammer::start(std::unique_ptr<HardwareManager::RfLock> rfLock, JammingMode mode, JammerConfig config) {
    if (isActive_) return false;
    if (!rfLock || !rfLock->isValid()) return false;
    
    rfLock_ = std::move(rfLock);
    Serial.printf("[JAMMER-LOG] Starting JammingMode %d with technique %d\n", (int)mode, (int)config.technique);
    
    isActive_ = true;
    currentMode_ = mode;
    currentConfig_ = config;
    channelHopIndex_ = 0;
    lastHopTime_ = 0;
    sweepIndex_ = 0;

    // Configure radios based on technique
    if (currentConfig_.technique == JammingTechnique::CONSTANT_CARRIER) {
        if (rfLock_->radio1) rfLock_->radio1->startConstCarrier(RF24_PA_MAX, 45);
        if (rfLock_->radio2) rfLock_->radio2->startConstCarrier(RF24_PA_MAX, 45);
    } else { // NOISE_INJECTION
        byte dummy_addr[5] = {0xE7, 0xE7, 0xE7, 0xE7, 0xE7};
        if (rfLock_->radio1) {
            rfLock_->radio1->openWritingPipe(dummy_addr);
            rfLock_->radio1->setPayloadSize(sizeof(jam_text));
        }
        if (rfLock_->radio2) {
            rfLock_->radio2->openWritingPipe(dummy_addr);
            rfLock_->radio2->setPayloadSize(sizeof(jam_text));
        }
    }
    
    // Initialize the first target WiFi channel for sweep attacks
    if (mode == JammingMode::WIFI_NARROWBAND) {
        targetWifiChannel_ = wifi_narrow_nrf_channels_[0];
    } else if (mode == JammingMode::CHANNEL_FLOOD_CUSTOM && !config.customChannels.empty()) {
        targetWifiChannel_ = config.customChannels[0];
    }

    Serial.printf("[JAMMER-LOG] Jammer started. Mode: %s\n", getModeString());
    return true;
}

void Jammer::stop() {
    if (!isActive_) return;
    Serial.println("[JAMMER-LOG] Stopping active jamming...");
    
    if (rfLock_) {
        if (currentConfig_.technique == JammingTechnique::CONSTANT_CARRIER) {
            if (rfLock_->radio1) rfLock_->radio1->stopConstCarrier();
            if (rfLock_->radio2) rfLock_->radio2->stopConstCarrier();
        }
        rfLock_.reset();
    }
    
    isActive_ = false;
    currentMode_ = JammingMode::IDLE;
    Serial.println("[JAMMER-LOG] Jammer stopped and RF lock released.");
}

void Jammer::loop() {
    if (!isActive_ || !rfLock_) return;

    switch(currentMode_) {
        case JammingMode::BLE: {
            size_t num_channels = sizeof(ble_adv_nrf_channels_)/sizeof(int);
            for (size_t i = 0; i < num_channels; ++i) {
                jamWithNoise(ble_adv_nrf_channels_[i], ble_adv_nrf_channels_[num_channels - 1 - i]);
            }
            break;
        }
        case JammingMode::BT_CLASSIC: {
            size_t num_channels = sizeof(bt_classic_nrf_channels_)/sizeof(int);
            for (size_t i = 0; i < num_channels; ++i) {
                jamWithConstantCarrier(bt_classic_nrf_channels_[i], bt_classic_nrf_channels_[num_channels - 1 - i]);
            }
            break;
        }
        case JammingMode::WIDE_SPECTRUM: {
            // This mode already used bracketing, but we'll make it a fast loop.
            for (int i = 0; i < 126; ++i) {
                jamWithConstantCarrier(i, 125 - i);
            }
            break;
        }
        case JammingMode::WIFI_NARROWBAND:
        case JammingMode::CHANNEL_FLOOD_CUSTOM: {
            const auto& channels = (currentMode_ == JammingMode::WIFI_NARROWBAND)
                                 ? std::vector<int>(wifi_narrow_nrf_channels_, wifi_narrow_nrf_channels_ + sizeof(wifi_narrow_nrf_channels_)/sizeof(int))
                                 : currentConfig_.customChannels;
            if (channels.empty()) return;

            int targetWifiChannel = channels[channelHopIndex_];
            
            // Barrage the selected WiFi channel using Spectrum Bracketing
            for (int i = (targetWifiChannel * 5) + 1; i < (targetWifiChannel * 5) + 23; i++) {
                // radio1 sweeps up, radio2 sweeps down from the opposite end of the full 2.4GHz spectrum
                jamWithNoise(i, 125 - i); 
            }

            unsigned long currentTime = millis();
            if (currentTime - lastHopTime_ > 50) { 
                channelHopIndex_ = (channelHopIndex_ + 1) % channels.size();
                lastHopTime_ = currentTime;
            }
            break;
        }
        default:
            break;
    }
}

void Jammer::jamWithNoise(int channel1, int channel2) {
    if (rfLock_->radio1 && channel1 >= 0 && channel1 <= 125) {
        rfLock_->radio1->setChannel(channel1);
        rfLock_->radio1->writeFast(&jam_text, sizeof(jam_text));
    }
    if (rfLock_->radio2 && channel2 >= 0 && channel2 <= 125) {
        rfLock_->radio2->setChannel(channel2);
        rfLock_->radio2->writeFast(&jam_text, sizeof(jam_text));
    }
}

void Jammer::jamWithConstantCarrier(int channel1, int channel2) {
    if (rfLock_->radio1 && channel1 >= 0 && channel1 <= 125) {
        rfLock_->radio1->setChannel(channel1);
    }
    if (rfLock_->radio2 && channel2 >= 0 && channel2 <= 125) {
        rfLock_->radio2->setChannel(channel2);
    }
}

bool Jammer::isActive() const { return isActive_; }
JammingMode Jammer::getCurrentMode() const { return currentMode_; }

const char* Jammer::getModeString() const {
    switch(currentMode_) {
        case JammingMode::BLE: return "BLE Noise";
        case JammingMode::BT_CLASSIC: return "BT Classic Barrage";
        case JammingMode::WIFI_NARROWBAND: return "WiFi Barrage";
        case JammingMode::WIDE_SPECTRUM: return "Wide Spectrum Barrage";
        case JammingMode::CHANNEL_FLOOD_CUSTOM: return "Custom Barrage";
        default: return "Idle";
    }
}