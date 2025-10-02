#include "Event.h"
#include "EventDispatcher.h"
#include "OtaManager.h"
#include "App.h"
#include "ConfigManager.h"
#include "WifiManager.h"
#include "SdCardManager.h"
#include "Config.h"
#include <ArduinoOTA.h>
#include <Update.h>

OtaManager::OtaManager() :
    app_(nullptr),
    wifiManager_(nullptr),
    webServer_(80),
    state_(OtaState::IDLE),
    uploadError_(false)
{
}

void OtaManager::setup(App* app) {
    app_ = app;
    wifiManager_ = &app->getWifiManager();
    loadCurrentFirmware();
    scanSdForFirmware();
    setupArduinoOta();
}

void OtaManager::loop() {
    switch (state_) {
        case OtaState::BASIC_ACTIVE:
            ArduinoOTA.handle();
            break;
        case OtaState::FLASHING:
            loopFlashing();
            break;
        default:
            break;
    }
}

void OtaManager::loopFlashing() {
    if (!uploadFile_) {
        statusMessage_ = "Flashing file error";
        state_ = OtaState::ERROR;
        return;
    }
    const size_t chunkSize = 4096;
    uint8_t buffer[chunkSize];
    size_t bytesRead = uploadFile_.read(buffer, chunkSize);
    if (bytesRead > 0) {
        if (Update.write(buffer, bytesRead) != bytesRead) {
            statusMessage_ = "Flash write failed";
            state_ = OtaState::ERROR;
            uploadFile_.close();
            Update.abort();
            return;
        }
        progress_.receivedBytes += bytesRead;
    }
    if (progress_.receivedBytes == progress_.totalBytes) {
        uploadFile_.close();
        if (!Update.end()) {
            statusMessage_ = "Finalization failed";
            state_ = OtaState::ERROR;
            return;
        }
        state_ = OtaState::SUCCESS;
        saveCurrentFirmware(pendingFwInfo_);
        enterTerminalState();
    }
}

OtaState OtaManager::getState() const { return state_; }
const OtaProgress& OtaManager::getProgress() const { return progress_; }
String OtaManager::getStatusMessage() const { return statusMessage_; }
String OtaManager::getDisplayIpAddress() const { return displayIpAddress_; }
const std::vector<FirmwareInfo>& OtaManager::getAvailableFirmwares() const { return availableSdFirmwares_; }

void OtaManager::stop() {
    if (state_ == OtaState::IDLE) return;
    Serial.println("[OTA-LOG] Stopping active OTA process.");

    if (state_ == OtaState::WEB_ACTIVE) {
        webServer_.end();
    }
    if (uploadFile_) {
        uploadFile_.close();
    }
    if (state_ == OtaState::FLASHING) {
        Update.abort();
    }
    resetState();
}

void OtaManager::resetState() {
    state_ = OtaState::IDLE;
    progress_.totalBytes = 0;
    progress_.receivedBytes = 0;
    statusMessage_ = "";
    displayIpAddress_ = "";
    uploadError_ = false;
    if (uploadFile_) {
        uploadFile_.close();
    }
}

void OtaManager::enterTerminalState() {
    if (!app_) return;
    U8G2& display = app_->getHardwareManager().getMainDisplay();
    IMenu* otaMenu = app_->getMenu(MenuType::OTA_STATUS);
    if (!otaMenu) return;

    statusMessage_ = "100% Complete";
    display.clearBuffer();
    app_->drawStatusBar();
    otaMenu->draw(app_, display);
    display.sendBuffer();
    delay(2500);

    statusMessage_ = "Rebooting...";
    display.clearBuffer();
    app_->drawStatusBar();
    otaMenu->draw(app_, display);
    display.sendBuffer();
    delay(1500);
    
    ESP.restart();
}

void OtaManager::loadCurrentFirmware() {
    if (FirmwareUtils::parseMetadataFile(SD_ROOT::CONFIG_CURRENT_FIRMWARE, currentFirmware_)) {
        Serial.printf("[OTA-LOG] Loaded current firmware: %s\n", currentFirmware_.version);
    } else {
        Serial.println("[OTA-LOG] Could not load current firmware info.");
        strcpy(currentFirmware_.version, "Unknown");
        currentFirmware_.isValid = false;
    }
}

void OtaManager::saveCurrentFirmware(const FirmwareInfo& info) {
    if (FirmwareUtils::saveMetadataFile(SD_ROOT::CONFIG_CURRENT_FIRMWARE, info)) {
        Serial.printf("[OTA-LOG] Saved current firmware info: %s\n", info.version);
        currentFirmware_ = info;
    } else {
        Serial.println("[OTA-LOG] Failed to save current firmware info.");
    }
}

void OtaManager::scanSdForFirmware() {
    availableSdFirmwares_.clear();
    if (!SdCardManager::getInstance().isAvailable() || !SdCardManager::getInstance().exists(SD_ROOT::FIRMWARE)) {
        return;
    }
    File root = SD.open(SD_ROOT::FIRMWARE);
    if (!root || !root.isDirectory()) {
        if(root) root.close();
        return;
    }
    File file = root.openNextFile();
    while(file && availableSdFirmwares_.size() < Firmware::MAX_FIRMWARES_ON_SD) {
        String fileName = file.name();
        if(!file.isDirectory() && fileName.endsWith(Firmware::METADATA_EXTENSION)) {
            FirmwareInfo info;
            if(FirmwareUtils::parseMetadataFile(file.path(), info)) {
                String binPath = String(SD_ROOT::FIRMWARE) + "/" + info.binary_filename;
                if(SD.exists(binPath)) {
                    availableSdFirmwares_.push_back(info);
                }
            }
        }
        file.close();
        file = root.openNextFile();
    }
    root.close();
}

const char* OtaManager::getEffectiveOtaPassword() {
    // This function centralizes the password logic.
    const char* configured_password = app_->getConfigManager().getSettings().otaPassword;
    
    // If the configured password is valid (long enough), use it.
    if (strlen(configured_password) >= Firmware::MIN_AP_PASSWORD_LEN) {
        return configured_password;
    }
    
    // Otherwise, fall back to the secure default.
    return "KIVA_PASS";
}

void OtaManager::setupArduinoOta() {
    ArduinoOTA.setHostname(Firmware::OTA_HOSTNAME);

    ArduinoOTA.setPassword(getEffectiveOtaPassword());

    ArduinoOTA.onStart([this]() {
        state_ = OtaState::BASIC_ACTIVE;
        statusMessage_ = "Receiving...";
        progress_.totalBytes = 0;
        progress_.receivedBytes = 0;
    });
    ArduinoOTA.onProgress([this](unsigned int progress, unsigned int total) {
        progress_.totalBytes = total;
        progress_.receivedBytes = progress;
    });
    ArduinoOTA.onEnd([this]() {
        state_ = OtaState::SUCCESS;
        progress_.receivedBytes = progress_.totalBytes;
        
        FirmwareInfo newFw;
        strcpy(newFw.version, "IDE Upload");
        snprintf(newFw.build_date, FW_BUILD_DATE_MAX_LEN, "%s %s", __DATE__, __TIME__);
        strcpy(newFw.checksum_md5, "N/A");
        saveCurrentFirmware(newFw);
        enterTerminalState();
    });
    ArduinoOTA.onError([this](ota_error_t error) {
        state_ = OtaState::ERROR;
        statusMessage_ = "Error: " + String(error);
        progress_.totalBytes = 0;
        progress_.receivedBytes = 1;
    });
}

void OtaManager::startBasicOta() {
    if (state_ != OtaState::IDLE) return;
    
    if (wifiManager_->getState() != WifiState::CONNECTED) {
        app_->showPopUp("WiFi Required", "Connect to use Basic OTA?", 
            [this](App* app_cb) {
                WifiListDataSource& wifiDataSource = app_cb->getWifiListDataSource();
                wifiDataSource.setBackNavOverride(true);
                wifiDataSource.setScanOnEnter(true);
                EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::WIFI_LIST));
            }, 
            "OK", "Cancel", false);
        return;
    }

    resetState();
    state_ = OtaState::BASIC_ACTIVE;

    statusMessage_ = "P: " + String(getEffectiveOtaPassword());
    
    displayIpAddress_ = WiFi.localIP().toString();
    ArduinoOTA.begin();
    Serial.println("[OTA-LOG] Basic OTA (IDE) started.");
    
    EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::OTA_STATUS));
}

void OtaManager::startSdUpdate(const FirmwareInfo& fwInfo) {
    if (state_ != OtaState::IDLE) return;
    resetState();
    state_ = OtaState::SD_UPDATING;
    statusMessage_ = "Preparing...";

    if (strcmp(fwInfo.checksum_md5, currentFirmware_.checksum_md5) == 0) {
        statusMessage_ = "Already up-to-date";
        state_ = OtaState::ERROR;
        return;
    }

    String binPath = String(SD_ROOT::FIRMWARE) + "/" + fwInfo.binary_filename;
    uploadFile_ = SD.open(binPath, FILE_READ);
    if (!uploadFile_) {
        statusMessage_ = "Firmware file error";
        state_ = OtaState::ERROR;
        return;
    }

    size_t updateSize = uploadFile_.size();
    if (updateSize == 0) {
        statusMessage_ = "Firmware file empty";
        state_ = OtaState::ERROR;
        uploadFile_.close();
        return;
    }
    
    if (!Update.begin(updateSize)) {
        statusMessage_ = "Update begin failed";
        state_ = OtaState::ERROR;
        uploadFile_.close();
        return;
    }

    pendingFwInfo_ = fwInfo;
    progress_.totalBytes = updateSize;
    progress_.receivedBytes = 0;
    statusMessage_ = "Flashing...";
    state_ = OtaState::FLASHING;
}

bool OtaManager::startWebUpdate() {
    if (state_ != OtaState::IDLE) return false;
    resetState();

    // --- START OF FIX ---
    // NEW: Get the effective password to create the secure AP.
    const char* ap_password = getEffectiveOtaPassword();
    
    char ssid[32];
    snprintf(ssid, sizeof(ssid), "%s-OTA", Firmware::OTA_HOSTNAME);
    
    wifiManager_->setHardwareState(true, WifiMode::AP, ssid, ap_password);
    if(!wifiManager_->isHardwareEnabled()) {
        statusMessage_ = "Failed to start AP";
        state_ = OtaState::ERROR;
        return false;
    }

    state_ = OtaState::WEB_ACTIVE;
    displayIpAddress_ = WiFi.softAPIP().toString();
    statusMessage_ = ap_password; // Display the effective password.
    // --- END OF FIX ---

    setupWebServer();
    webServer_.begin();
    Serial.println("[OTA-LOG] Web OTA update server started.");
    return true;
}

void OtaManager::setupWebServer() {
    webServer_.on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
        String path = SD_ROOT::WEB_OTA_PAGE;
        if (SdCardManager::getInstance().exists(path.c_str())) {
            request->send(SD, path, "text/html");
        } else {
            request->send(404, "text/plain", "OTA Page not found on SD card.");
        }
    });

    webServer_.on("/update", HTTP_POST, 
        [this](AsyncWebServerRequest *request) { this->onUpdateEnd(request); },
        [this](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) { this->onUpload(request, filename, index, data, len, final); }
    );
}

void OtaManager::onUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
    if (index == 0) {
        Serial.printf("[OTA-WEB] Upload Start: %s\n", filename.c_str());
        statusMessage_ = "Receiving File...";
        progress_.totalBytes = request->contentLength();
        progress_.receivedBytes = 0;
        
        String tempPath = String(SD_ROOT::FIRMWARE) + "/web_upload.bin";
        if (SD.exists(tempPath)) SD.remove(tempPath);
        uploadFile_ = SD.open(tempPath, FILE_WRITE);
        if (!uploadFile_) {
            uploadError_ = true;
            statusMessage_ = "SD: Cannot create temp file.";
            return;
        }
    }
    
    if (!uploadError_ && uploadFile_ && len > 0) {
        if (uploadFile_.write(data, len) != len) {
            uploadError_ = true;
            statusMessage_ = "SD: Write failed.";
        }
        progress_.receivedBytes = index + len;
    }

    if (final) {
        if (uploadFile_) uploadFile_.close();
        Serial.println("[OTA-WEB] Upload finished.");
    }
}

void OtaManager::onUpdateEnd(AsyncWebServerRequest *request) {
    if (uploadError_) {
        request->send(500, "text/plain", statusMessage_);
        state_ = OtaState::ERROR;
        return;
    }

    statusMessage_ = "Verifying...";
    String tempPath = String(SD_ROOT::FIRMWARE) + "/web_upload.bin";
    String md5 = FirmwareUtils::calculateFileMD5(SD, tempPath);

    if (md5.isEmpty()) {
        SdCardManager::getInstance().deleteFile(tempPath.c_str());
        statusMessage_ = "MD5 Calc Failed";
        state_ = OtaState::ERROR;
        request->send(200, "text/plain", statusMessage_);
        return;
    }
    
    if (currentFirmware_.isValid && md5.equalsIgnoreCase(currentFirmware_.checksum_md5)) {
        SdCardManager::getInstance().deleteFile(tempPath.c_str());
        statusMessage_ = "Already up-to-date";
        state_ = OtaState::ERROR;
        request->send(200, "text/plain", statusMessage_);
        return;
    }

    String pathToFlash = tempPath;
    
    unsigned long timestamp = millis();
    String baseFilename = "fw_" + String(timestamp);
    String permanentBinFilename = baseFilename + ".bin";
    String permanentBinPath = String(SD_ROOT::FIRMWARE) + "/" + permanentBinFilename;

    if (SdCardManager::getInstance().renameFile(tempPath.c_str(), permanentBinPath.c_str())) {
        Serial.printf("[OTA-WEB] Saved new firmware as %s\n", permanentBinPath.c_str());
        
        FirmwareInfo newFwInfo;
        snprintf(newFwInfo.version, FW_VERSION_MAX_LEN, "Web Upload %lu", timestamp);
        snprintf(newFwInfo.build_date, FW_BUILD_DATE_MAX_LEN, "%s %s", __DATE__, __TIME__);
        strcpy(newFwInfo.checksum_md5, md5.c_str());
        strcpy(newFwInfo.binary_filename, permanentBinFilename.c_str());
        strcpy(newFwInfo.description, "Uploaded via web interface");
        newFwInfo.isValid = true;

        String kfwPath = String(SD_ROOT::FIRMWARE) + "/" + baseFilename + Firmware::METADATA_EXTENSION;
        FirmwareUtils::saveMetadataFile(kfwPath, newFwInfo);
        
        pathToFlash = permanentBinPath;
    } else {
        Serial.println("[OTA-WEB] ERROR: Failed to rename temp file. Will flash from temp path without saving.");
    }

    uploadFile_ = SD.open(pathToFlash, FILE_READ);
    if (!uploadFile_) {
        state_ = OtaState::ERROR;
        statusMessage_ = "Failed to open file for flashing";
        request->send(500, "text/plain", statusMessage_);
        return;
    }
    
    size_t updateSize = uploadFile_.size();
    if (!Update.begin(updateSize)) {
        statusMessage_ = "Update begin failed";
        state_ = OtaState::ERROR;
        uploadFile_.close();
        request->send(500, "text/plain", statusMessage_);
        return;
    }

    request->send(200, "text/plain", "Success");

    snprintf(pendingFwInfo_.version, FW_VERSION_MAX_LEN, "Web Upload %lu", timestamp);
    snprintf(pendingFwInfo_.build_date, FW_BUILD_DATE_MAX_LEN, "%s %s", __DATE__, __TIME__);
    strcpy(pendingFwInfo_.checksum_md5, md5.c_str());
    strcpy(pendingFwInfo_.binary_filename, permanentBinFilename.c_str());
    strcpy(pendingFwInfo_.description, "Uploaded via web interface");
    
    progress_.totalBytes = updateSize;
    progress_.receivedBytes = 0;
    statusMessage_ = "Flashing...";
    state_ = OtaState::FLASHING;
}
