#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <vector>

// Forward declaration
class App;

// A struct to hold all persistent settings
struct DeviceSettings {
    // REMOVED: uint8_t unifiedBrightness;
    uint8_t mainDisplayBrightness; // 0-255
    uint8_t auxDisplayBrightness;  // 0-255
    int keyboardLayoutIndex;
    char otaPassword[33]; // Max 32 chars + null terminator
};

class ConfigManager {
public:
    ConfigManager();
    void setup(App* app);
    
    // Load/Save operations
    void loadSettings();
    void saveSettings();
    bool reloadFromSdCard();

    // Accessors
    DeviceSettings& getSettings();
    const std::vector<std::pair<std::string, std::string>>& getKeyboardLayouts() const;

private:
    void applySettings();
    void loadFromEEPROM();
    void saveToEEPROM();
    void loadFromSdCard();
    void saveToSdCard();
    void useDefaultSettings();

    App* app_;
    DeviceSettings settings_;
    bool isEepromValid_;

    std::vector<std::pair<std::string, std::string>> keyboardLayouts_;
    static const uint32_t EEPROM_MAGIC_NUMBER = 0xDEADBEEF;
};

#endif // CONFIG_MANAGER_H