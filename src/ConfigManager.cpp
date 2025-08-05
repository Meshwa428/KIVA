#include "ConfigManager.h"
#include "App.h"
#include "Logger.h"
#include "SdCardManager.h"
#include <EEPROM.h>
#include <ArduinoJson.h>

#define EEPROM_SIZE 64 // Allocate 64 bytes for settings

const uint32_t ConfigManager::EEPROM_MAGIC_NUMBER;

ConfigManager::ConfigManager() :
    app_(nullptr),
    isEepromValid_(false)
{
    // Define the available keyboard layouts
    keyboardLayouts_ = {
        {"US", "en_US"},
        {"UK", "en_UK"},
        {"DE", "de_DE"},
        {"FR", "fr_FR"},
        {"ES", "es_ES"},
        {"PT", "pt_PT"}
        // Add more layouts here as they are created
    };
}

void ConfigManager::setup(App* app) {
    app_ = app;
    EEPROM.begin(EEPROM_SIZE);
    loadSettings();
    applySettings();
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

void ConfigManager::saveSettings() {
    saveToEEPROM();
    saveToSdCard();
    applySettings();
}

void ConfigManager::applySettings() {
    // Apply settings that need to be pushed to hardware
    // --- UPDATE THIS LOGIC ---
    app_->getHardwareManager().setMainBrightness(settings_.mainDisplayBrightness);
    app_->getHardwareManager().setAuxBrightness(settings_.auxDisplayBrightness);
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
    if (!SdCardManager::exists(path)) {
        isEepromValid_ = false;
        return;
    }

    String jsonStr = SdCardManager::readFile(path);
    JsonDocument doc;
    if (deserializeJson(doc, jsonStr)) {
        isEepromValid_ = false;
        return;
    }

    // REMOVED: settings.unifiedBrightness = doc["unified_brightness"] | 255;
    settings_.mainDisplayBrightness = doc["main_brightness"] | 255;
    settings_.auxDisplayBrightness = doc["aux_brightness"] | 255;
    settings_.keyboardLayoutIndex = doc["kb_layout_idx"] | 0;
    strlcpy(settings_.otaPassword, doc["ota_password"] | "KIVA_PASS", sizeof(settings_.otaPassword));
    
    if (settings_.keyboardLayoutIndex >= keyboardLayouts_.size()) {
        settings_.keyboardLayoutIndex = 0;
    }
    
    isEepromValid_ = true;
}

void ConfigManager::saveToSdCard() {
    const char* path = "/config/settings.json";
    JsonDocument doc;
    // REMOVED: doc["unified_brightness"] = settings.unifiedBrightness;
    doc["main_brightness"] = settings_.mainDisplayBrightness;
    doc["aux_brightness"] = settings_.auxDisplayBrightness;
    doc["kb_layout_idx"] = settings_.keyboardLayoutIndex;
    doc["ota_password"] = settings_.otaPassword;

    String jsonStr;
    serializeJson(doc, jsonStr);
    SdCardManager::writeFile(path, jsonStr.c_str());
}

void ConfigManager::useDefaultSettings() {
    // REMOVED: settings.unifiedBrightness = 255;
    settings_.mainDisplayBrightness = 255;
    settings_.auxDisplayBrightness = 255;
    settings_.keyboardLayoutIndex = 0; 
    strcpy(settings_.otaPassword, "KIVA_PASS");
}

bool ConfigManager::reloadFromSdCard() {
    // Attempt to load settings from the SD card.
    // The private loadFromSdCard() function sets the isEepromValid_ flag on success.
    loadFromSdCard();
    
    if (isEepromValid_) {
        // If the load was successful, sync the newly loaded settings to the EEPROM.
        LOG(LogLevel::INFO, "CONFIG", "Manually reloaded from SD. Syncing to EEPROM.");
        saveToEEPROM();
        // And apply them immediately to the hardware (e.g., brightness).
        applySettings();
        return true; // Report success
    } else {
        // If the load failed (e.g., file not found or corrupt), report failure.
        LOG(LogLevel::WARN, "CONFIG", "Manual reload from SD failed. File might be invalid.");
        return false;
    }
}

DeviceSettings& ConfigManager::getSettings() {
    return settings_;
}

const std::vector<std::pair<std::string, std::string>>& ConfigManager::getKeyboardLayouts() const {
    return keyboardLayouts_;
}