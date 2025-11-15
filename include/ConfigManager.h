#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <vector>
#include <cstdint> // For uint8_t

// Forward declaration
class App;

// A struct to hold all persistent settings
struct DeviceSettings {
    uint8_t mainDisplayBrightness; // 0-255
    uint8_t auxDisplayBrightness;  // 0-255
    uint8_t volume;                // 0-200, representing 0-200%
    int keyboardLayoutIndex;
    char otaPassword[33]; // Max 32 chars + null terminator
    int channelHopDelayMs; // in milliseconds
    int attackCooldownMs;  // Cooldown for broadcast attacks, in milliseconds
    uint32_t secondaryWidgetMask; // Bitmask for secondary display widgets
    char timezoneString[40];      // <-- MODIFIED: From int32_t to char array
};

#include "Service.h"

class ConfigManager : public Service {
public:
    ConfigManager();
    void setup(App* app) override;
    
    // Load/Save operations
    void loadSettings();
    void saveSettings();
    bool reloadFromSdCard();

    // Accessors
    DeviceSettings& getSettings();
    const DeviceSettings& getSettings() const;
    const std::vector<std::pair<std::string, std::string>>& getKeyboardLayouts() const;
    const uint8_t* getSelectedKeyboardLayout() const;

    void requestSave();

    void applySettings();

private:
    void loadFromEEPROM();
    void saveToEEPROM();
    void loadFromSdCard();
    void saveToSdCard();
    void useDefaultSettings();

    App* app_;
    DeviceSettings settings_;
    DeviceSettings lastAppliedSettings_;
    bool isEepromValid_;
    
    // --- NEW: Flag to track unsaved changes ---
    bool saveRequired_; 

    std::vector<std::pair<std::string, std::string>> keyboardLayouts_;
    static constexpr uint32_t EEPROM_MAGIC_NUMBER = 0xDEADBEEF;
};

#endif // CONFIG_MANAGER_H