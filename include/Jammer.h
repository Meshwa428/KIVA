#ifndef JAMMER_H
#define JAMMER_H

#include "Config.h"
#include "HardwareManager.h"
#include <vector>
#include <memory>  // For std::unique_ptr

#define SPI_SPEED_NRF 16000000 // SPI speed for NRF modules

// Forward declaration
class App;

// Defines the different jamming strategies
enum class JammingMode {
    IDLE,
    BLE,
    BT_CLASSIC,
    WIFI_NARROWBAND,
    WIDE_SPECTRUM,
    CHANNEL_FLOOD_CUSTOM,
    ZIGBEE
};

// Defines the underlying RF transmission technique
enum class JammingTechnique {
    NOISE_INJECTION, // Sending garbage packets
    CONSTANT_CARRIER // Broadcasting an unmodulated carrier wave
};

// A structure to pass configuration data
struct JammerConfig {
    std::vector<int> customChannels;
    JammingTechnique technique = JammingTechnique::NOISE_INJECTION;
};

#include "Service.h"

class Jammer : public Service {
public:
    Jammer();
    void setup(App* app) override;

    bool start(std::unique_ptr<HardwareManager::RfLock> rfLock, JammingMode mode, JammerConfig config = {});
    void stop();
    void loop();

    bool isActive() const;
    JammingMode getCurrentMode() const;
    const char* getModeString() const;

private:
    void jamWithNoise(int channel1, int channel2);
    void jamWithConstantCarrier(int channel1, int channel2);

    App* app_;
    std::unique_ptr<HardwareManager::RfLock> rfLock_;

    // Jamming State
    bool isActive_;
    JammingMode currentMode_;
    JammerConfig currentConfig_;

    // Channel-hopping logic state
    int channelHopIndex_;
    unsigned long lastHopTime_;

    // --- STATE FOR STEP-BY-STEP SWEEPING ---
    int sweepIndex_; 
    int targetWifiChannel_;

    // Pre-defined channel lists for different modes
    static const int ble_adv_nrf_channels_[];
    static const int bt_classic_nrf_channels_[];
    static const int wifi_narrow_nrf_channels_[];
};

#endif // JAMMER_H