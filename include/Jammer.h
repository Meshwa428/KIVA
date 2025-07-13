#ifndef JAMMER_H
#define JAMMER_H

#include "Config.h"
#include "HardwareManager.h"
#include <vector>
#include <memory>  // For std::unique_ptr

#define SPI_SPEED_NRF 16000000 // SPI speed for NRF modules

// Forward declaration to avoid circular dependencies
class App;

// Defines the different jamming strategies the class can execute.
enum class JammingMode {
    IDLE,
    BLE,                // Bluetooth Low Energy advertising channels
    BT_CLASSIC,         // Bluetooth Classic frequency hopping channels
    WIFI_NARROWBAND,    // 2.4 GHz WiFi channels (1-14)
    WIDE_SPECTRUM,      // Rapidly sweeps all 125 NRF channels
    CHANNEL_FLOOD_CUSTOM// Jams a user-provided list of channels
};

// A structure to pass configuration data, primarily for custom modes.
struct JammerConfig {
    std::vector<int> customChannels;
};

class Jammer {
public:
    Jammer();
    void setup(App* app);

    /**
     * @brief Starts a jamming operation. This is the main entry point.
     * This function will request a full hardware takeover of RF functionalities.
     * @param mode The jamming strategy to use.
     * @param config Configuration for the selected mode (e.g., custom channels).
     * @return true if jamming was successfully started, false otherwise.
     */
    bool start(std::unique_ptr<HardwareManager::RfLock> rfLock, JammingMode mode, JammerConfig config = {});
    
    /**
     * @brief Stops any active jamming operation and releases RF hardware.
     */
    void stop();

    /**
     * @brief The main update loop for the jammer, called repeatedly.
     * Performs the channel hopping and RF transmission for the active mode.
     */
    void loop();

    // --- State Getters ---
    bool isActive() const;
    JammingMode getCurrentMode() const;
    const char* getModeString() const;

private:
    // Helper function to initialize and configure an NRF24 module.
    bool configureRadio(RF24& radio, const char* radioName);

    App* app_; // pointer to the main application context

    std::unique_ptr<HardwareManager::RfLock> rfLock_;

    // Jamming State
    bool isActive_;
    JammingMode currentMode_;
    JammerConfig currentConfig_;

    // Channel-hopping logic state
    int channelHopIndex_;

    // Pre-defined channel lists for different modes
    static const int ble_adv_nrf_channels_[];
    static const int bt_classic_nrf_channels_[];
    static const int wifi_narrow_nrf_channels_[];
};

#endif // JAMMER_H