#include "ConfigManager.h"
#include "App.h"
#include "Logger.h"
#include "SdCardManager.h"
#include <EEPROM.h>
#include <ArduinoJson.h>
#include <HIDForge.h>

#define EEPROM_SIZE 128 // Increase EEPROM size to accommodate the longer string

ConfigManager::ConfigManager() :
    app_(nullptr),
    isEepromValid_(false),
    saveRequired_(false) // Initialize the new flag
{
    // Initialize both structs to a known state
    memset(&settings_, 0, sizeof(DeviceSettings));
    memset(&lastAppliedSettings_, 0, sizeof(DeviceSettings));

    // Define the available keyboard layouts
    keyboardLayouts_ = {
        {"US", "en_US"},
        {"UK", "en_UK"},
        {"DE", "de_DE"},
        {"FR", "fr_FR"},
        {"ES", "es_ES"},
        {"PT", "pt_PT"}
    };
}

void ConfigManager::setup(App* app) {
    app_ = app;
    EEPROM.begin(EEPROM_SIZE);
    loadSettings();
    applySettings();
    // After the first load and apply, sync the shadow copy
    lastAppliedSettings_ = settings_;
}

const uint8_t* ConfigManager::getSelectedKeyboardLayout() const {
    if (settings_.keyboardLayoutIndex < 0 || (size_t)settings_.keyboardLayoutIndex >= keyboardLayouts_.size()) {
        return KeyboardLayout_en_US; // Safe default
    }

    const std::string& name = keyboardLayouts_[settings_.keyboardLayoutIndex].first;
    if (name == "US") return KeyboardLayout_en_US;
    if (name == "UK") return KeyboardLayout_en_UK;
    if (name == "DE") return KeyboardLayout_de_DE;
    if (name == "FR") return KeyboardLayout_fr_FR;
    if (name == "ES") return KeyboardLayout_es_ES;
    if (name == "PT") return KeyboardLayout_pt_PT;
    
    return KeyboardLayout_en_US; // Fallback default
}

void ConfigManager::loadSettings() {
    loadFromEEPROM();
    if (isEepromValid_) {
        LOG(LogLevel::INFO, "CONFIG", "Loaded settings from EEPROM.");
        return;
    }

    loadFromSdCard();
    if (isEepromValid_) { // loadFromSdCard sets this flag on success
        LOG(LogLevel::INFO, "CONFIG", "Loaded settings from SD Card, saving to EEPROM.");
        saveToEEPROM();
        return;
    }
    
    LOG(LogLevel::WARN, "CONFIG", "No valid settings found. Using defaults and saving.");
    useDefaultSettings();
    saveSettings(); // Save defaults to both EEPROM and SD
}

void ConfigManager::requestSave() {
    saveRequired_ = true;
}

void ConfigManager::saveSettings() {
    // Only apply settings if they have actually changed.
    applySettings();

    // Only save to storage if a save has been requested.
    if (saveRequired_) {
        LOG(LogLevel::INFO, "CONFIG", "Saving dirty settings to EEPROM and SD Card...");
        saveToEEPROM();
        saveToSdCard();
        saveRequired_ = false; // Reset the flag after saving
    }
    
    // After saving and applying, update the shadow copy to the new state
    lastAppliedSettings_ = settings_;
}

void ConfigManager::applySettings() {
    // --- START: NEW INTELLIGENT APPLY LOGIC ---
    LOG(LogLevel::DEBUG, "CONFIG", false, "Checking for settings changes to apply...");

    // Compare and apply brightness settings
    if (settings_.mainDisplayBrightness != lastAppliedSettings_.mainDisplayBrightness) {
        LOG(LogLevel::INFO, "CONFIG", "Applying new Main Brightness: %d", settings_.mainDisplayBrightness);
        app_->getHardwareManager().setMainBrightness(settings_.mainDisplayBrightness);
    }
    if (settings_.auxDisplayBrightness != lastAppliedSettings_.auxDisplayBrightness) {
        LOG(LogLevel::INFO, "CONFIG", "Applying new Aux Brightness: %d", settings_.auxDisplayBrightness);
        app_->getHardwareManager().setAuxBrightness(settings_.auxDisplayBrightness);
    }

    // Compare and apply volume setting
    if (settings_.volume != lastAppliedSettings_.volume) {
        LOG(LogLevel::INFO, "CONFIG", "Applying new Volume: %d%%", settings_.volume);
        app_->getMusicPlayer().setVolume(settings_.volume);
    }

    // Settings like keyboard layout, passwords, and hop delay don't need an explicit "apply" function
    // as they are read by other modules when needed. No action is required here for them.
    // --- END: NEW INTELLIGENT APPLY LOGIC ---
}

void ConfigManager::loadFromEEPROM() {
    uint32_t magic;
    EEPROM.get(0, magic);

    if (magic != EEPROM_MAGIC_NUMBER) {
        isEepromValid_ = false;
        return;
    }

    EEPROM.get(sizeof(uint32_t), settings_);
    settings_.otaPassword[sizeof(settings_.otaPassword) - 1] = '\0';
    settings_.timezoneString[sizeof(settings_.timezoneString) - 1] = '\0'; // Ensure null termination
    isEepromValid_ = true;
}

void ConfigManager::saveToEEPROM() {
    EEPROM.put(0, EEPROM_MAGIC_NUMBER);
    EEPROM.put(sizeof(uint32_t), settings_);
    EEPROM.commit();
    isEepromValid_ = true;
}

void ConfigManager::loadFromSdCard() {
    const char* path = "/config/settings.json";
    if (!SdCardManager::getInstance().exists(path)) {
        isEepromValid_ = false;
        return;
    }

    String jsonStr = SdCardManager::getInstance().readFile(path);
    JsonDocument doc;
    if (deserializeJson(doc, jsonStr)) {
        isEepromValid_ = false;
        return;
    }

    settings_.mainDisplayBrightness = doc["main_brightness"] | 255;
    settings_.auxDisplayBrightness = doc["aux_brightness"] | 255;
    settings_.volume = doc["volume"] | 100;
    settings_.keyboardLayoutIndex = doc["kb_layout_idx"] | 0;
    strlcpy(settings_.otaPassword, doc["ota_password"] | "KIVA_PASS", sizeof(settings_.otaPassword));
    settings_.channelHopDelayMs = doc["channel_hop_delay_ms"] | 500;
    settings_.attackCooldownMs = doc["attack_cooldown_ms"] | 30000; // Default to 30 seconds
    settings_.secondaryWidgetMask = doc["widget_mask"] | 9; // Default to RAM (bit 0) and CPU (bit 3)
    strlcpy(settings_.timezoneString, doc["timezone_string"] | "UTC0", sizeof(settings_.timezoneString)); // <-- MODIFIED
    
    if ((size_t)settings_.keyboardLayoutIndex >= keyboardLayouts_.size()) {
        settings_.keyboardLayoutIndex = 0;
    }
    
    isEepromValid_ = true;
}

void ConfigManager::saveToSdCard() {
    const char* path = "/config/settings.json";
    JsonDocument doc;
    doc["main_brightness"] = settings_.mainDisplayBrightness;
    doc["aux_brightness"] = settings_.auxDisplayBrightness;
    doc["volume"] = settings_.volume;
    doc["kb_layout_idx"] = settings_.keyboardLayoutIndex;
    doc["ota_password"] = settings_.otaPassword;
    doc["channel_hop_delay_ms"] = settings_.channelHopDelayMs;
    doc["attack_cooldown_ms"] = settings_.attackCooldownMs;
    doc["widget_mask"] = settings_.secondaryWidgetMask;
    doc["timezone_string"] = settings_.timezoneString; // <-- MODIFIED

    String jsonStr;
    serializeJson(doc, jsonStr);
    SdCardManager::getInstance().writeFile(path, jsonStr.c_str());
}

void ConfigManager::useDefaultSettings() {
    settings_.mainDisplayBrightness = 255;
    settings_.auxDisplayBrightness = 255;
    settings_.volume = 100;
    settings_.keyboardLayoutIndex = 0; 
    strcpy(settings_.otaPassword, "KIVA_PASS");
    settings_.channelHopDelayMs = 500;
    settings_.attackCooldownMs = 1; // 1 seconds
    settings_.secondaryWidgetMask = 9; // Default to RAM (bit 0) and CPU (bit 3)
    strcpy(settings_.timezoneString, "UTC0"); // <-- MODIFIED
}

bool ConfigManager::reloadFromSdCard() {
    loadFromSdCard();
    
    if (isEepromValid_) {
        LOG(LogLevel::INFO, "CONFIG", "Manually reloaded from SD. Syncing to EEPROM.");
        saveToEEPROM();
        applySettings();
        // After applying, re-sync the shadow copy.
        lastAppliedSettings_ = settings_;
        return true;
    } else {
        LOG(LogLevel::WARN, "CONFIG", "Manual reload from SD failed. File might be invalid.");
        return false;
    }
}

DeviceSettings& ConfigManager::getSettings() {
    return settings_;
}

const DeviceSettings& ConfigManager::getSettings() const {
    return settings_;
}

const std::vector<std::pair<std::string, std::string>>& ConfigManager::getKeyboardLayouts() const {
    return keyboardLayouts_;
}