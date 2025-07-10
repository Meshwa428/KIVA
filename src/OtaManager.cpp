#include "OtaManager.h"
#include "App.h"
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
    uploadError_(false),
    rebootPending_(false), // Initialize new member
    rebootScheduledTime_(0) // Initialize new member
{
    // The progress_ struct is default-initialized to zeros.
}

void OtaManager::setup(App* app, WifiManager* wifiManager) {
    app_ = app;
    wifiManager_ = wifiManager;
    loadCurrentFirmware();
    scanSdForFirmware(); // Scan once at startup
    setupArduinoOta();
}

void OtaManager::loop() {
    if (state_ == OtaState::BASIC_ACTIVE) {
        ArduinoOTA.handle();
    }
    
    // --- NEW: Handle the delayed reboot ---
    if (rebootPending_ && (millis() - rebootScheduledTime_ > 2000)) {
        ESP.restart();
    }
}

OtaState OtaManager::getState() const { return state_; }
const OtaProgress& OtaManager::getProgress() const { return progress_; }
String OtaManager::getStatusMessage() const { return statusMessage_; }
String OtaManager::getDisplayIpAddress() const { return displayIpAddress_; }
const std::vector<FirmwareInfo>& OtaManager::getAvailableFirmwares() const { return availableSdFirmwares_; }

void OtaManager::stop() {
    if (state_ == OtaState::IDLE) return; // Do nothing if already idle.
    
    Serial.println("[OTA-LOG] Stopping active OTA process.");
    if (state_ == OtaState::WEB_ACTIVE) {
        webServer_.end();
    }
    // Don't turn off WiFi here, let App::loop manage it.
    resetState();
}

void OtaManager::resetState() {
    state_ = OtaState::IDLE;
    progress_.totalBytes = 0;
    progress_.receivedBytes = 0;
    statusMessage_ = "";
    displayIpAddress_ = "";
    uploadError_ = false;
    rebootPending_ = false; // Reset the flag
    rebootScheduledTime_ = 0;
    if (uploadFile_) {
        uploadFile_.close();
    }
}

void OtaManager::scheduleReboot() {
    if (!app_) return;

    // This function now just sets the flag and timestamp.
    // It does NOT block or reboot directly.
    statusMessage_ = "Success! Rebooting...";
    rebootPending_ = true;
    rebootScheduledTime_ = millis();
}

// --- Firmware Info Management ---

void OtaManager::loadCurrentFirmware() {
    if (FirmwareUtils::parseMetadataFile(Firmware::CURRENT_FIRMWARE_INFO_FILENAME, currentFirmware_)) {
        Serial.printf("[OTA-LOG] Loaded current firmware: %s\n", currentFirmware_.version);
    } else {
        Serial.println("[OTA-LOG] Could not load current firmware info.");
        strcpy(currentFirmware_.version, "Unknown");
        currentFirmware_.isValid = false;
    }
}

void OtaManager::saveCurrentFirmware(const FirmwareInfo& info) {
    if (FirmwareUtils::saveMetadataFile(Firmware::CURRENT_FIRMWARE_INFO_FILENAME, info)) {
        Serial.printf("[OTA-LOG] Saved current firmware info: %s\n", info.version);
        currentFirmware_ = info;
    } else {
        Serial.println("[OTA-LOG] Failed to save current firmware info.");
    }
}

void OtaManager::scanSdForFirmware() {
    availableSdFirmwares_.clear();
    if (!SdCardManager::isAvailable() || !SdCardManager::exists(Firmware::FIRMWARE_DIR_PATH)) {
        return;
    }
    File root = SD.open(Firmware::FIRMWARE_DIR_PATH);
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
                String binPath = String(Firmware::FIRMWARE_DIR_PATH) + "/" + info.binary_filename;
                if(SD.exists(binPath)) {
                    availableSdFirmwares_.push_back(info);
                }
            }
        }
        file.close();
        file = root.openNextFile();
    }
    root.close();
    Serial.printf("[OTA-LOG] Found %d valid firmwares on SD card.\n", availableSdFirmwares_.size());
}


// --- Basic OTA (IDE) ---

void OtaManager::setupArduinoOta() {
    ArduinoOTA.setHostname(Firmware::OTA_HOSTNAME);

    String password = SdCardManager::readFile(Firmware::OTA_AP_PASSWORD_FILE);
    password.trim();
    if (password.length() >= Firmware::MIN_AP_PASSWORD_LEN) {
        ArduinoOTA.setPassword(password.c_str());
    }

    ArduinoOTA.onStart([this]() {
        state_ = OtaState::BASIC_ACTIVE;
        statusMessage_ = "Receiving...";
        progress_.totalBytes = 0; // Total is unknown at start for basic
        progress_.receivedBytes = 0;
    });
    ArduinoOTA.onProgress([this](unsigned int progress, unsigned int total) {
        // Here we finally get the total size
        progress_.totalBytes = total;
        progress_.receivedBytes = progress;
    });
    ArduinoOTA.onEnd([this]() {
        state_ = OtaState::SUCCESS;
        progress_.receivedBytes = progress_.totalBytes; // Mark as 100%
        
        FirmwareInfo newFw;
        strcpy(newFw.version, "IDE Upload");
        snprintf(newFw.build_date, FW_BUILD_DATE_MAX_LEN, "%s %s", __DATE__, __TIME__);
        strcpy(newFw.checksum_md5, "N/A");
        saveCurrentFirmware(newFw);

        scheduleReboot(); // <-- Use new function
    });
    ArduinoOTA.onError([this](ota_error_t error) {
        state_ = OtaState::ERROR;
        statusMessage_ = "Error: " + String(error);
        progress_.totalBytes = 0; // Mark as error
        progress_.receivedBytes = 1; // Differentiate from idle
    });
}

bool OtaManager::startBasicOta() {
    if (state_ != OtaState::IDLE) return false;
    
    if (wifiManager_->getState() != WifiState::CONNECTED) {
        statusMessage_ = "WiFi Not Connected";
        progress_.totalBytes = 0;
        progress_.receivedBytes = 1; 

        app_->setPostWifiAction([](App* app_ptr) {
            if (app_ptr->getOtaManager().startBasicOta()) {
                app_ptr->returnToMenu(MenuType::FIRMWARE_UPDATE_GRID);
                app_ptr->changeMenu(MenuType::OTA_STATUS);
            }
        });

        app_->showPopUp("WiFi Required", "Connect to use Basic OTA?", [this](App* app_cb) {
            WifiListMenu* wifiMenu = static_cast<WifiListMenu*>(app_cb->getMenu(MenuType::WIFI_LIST));
            if (wifiMenu) {
                wifiMenu->setScanOnEnter(true);
            }
            app_cb->changeMenu(MenuType::WIFI_LIST);
        });
        
        return false;
    }
    
    resetState();
    state_ = OtaState::BASIC_ACTIVE;
    statusMessage_ = "Waiting for IDE";
    displayIpAddress_ = WiFi.localIP().toString();
    ArduinoOTA.begin();
    Serial.println("[OTA-LOG] Basic OTA (IDE) started.");
    return true;
}


// --- SD Card OTA ---

void OtaManager::startSdUpdate(const FirmwareInfo& fwInfo) {
    if (state_ != OtaState::IDLE) return;
    resetState();
    state_ = OtaState::SD_UPDATING;
    statusMessage_ = "Preparing...";

    if (strcmp(fwInfo.checksum_md5, currentFirmware_.checksum_md5) == 0) {
        statusMessage_ = "Already running this version.";
        state_ = OtaState::ERROR;
        return;
    }

    String binPath = String(Firmware::FIRMWARE_DIR_PATH) + "/" + fwInfo.binary_filename;
    File updateBin = SD.open(binPath, FILE_READ);
    if (!updateBin || updateBin.isDirectory() || updateBin.size() == 0) {
        statusMessage_ = "Firmware file error";
        state_ = OtaState::ERROR;
        if (updateBin) updateBin.close();
        return;
    }

    size_t updateSize = updateBin.size();
    progress_.totalBytes = updateSize;
    progress_.receivedBytes = 0;

    statusMessage_ = "Flashing...";
    Update.onProgress([this](size_t uploaded, size_t total){
        progress_.receivedBytes = uploaded;
    });

    Update.setMD5(fwInfo.checksum_md5);
    if (!Update.begin(updateSize)) {
        statusMessage_ = "Update begin failed";
        state_ = OtaState::ERROR;
        updateBin.close();
        return;
    }
    
    size_t written = Update.writeStream(updateBin);
    updateBin.close();

    if (written != updateSize) {
        statusMessage_ = "Flash write failed";
        state_ = OtaState::ERROR;
        return;
    }

    if (!Update.end()) {
        statusMessage_ = "Finalization failed";
        state_ = OtaState::ERROR;
        return;
    }

    state_ = OtaState::SUCCESS;
    progress_.receivedBytes = progress_.totalBytes;
    saveCurrentFirmware(fwInfo);
    scheduleReboot(); // <-- Use new function
}

// --- Web OTA ---

bool OtaManager::startWebUpdate() {
    if (state_ != OtaState::IDLE) return false;
    resetState();
    String password = SdCardManager::readFile(Firmware::OTA_AP_PASSWORD_FILE);
    password.trim();
    if (password.length() < Firmware::MIN_AP_PASSWORD_LEN) {
        password = "";
    }
    
    char ssid[32];
    snprintf(ssid, sizeof(ssid), "%s-OTA", Firmware::OTA_HOSTNAME);
    
    wifiManager_->setHardwareState(true, WifiMode::AP, ssid, password.c_str());
    if(!wifiManager_->isHardwareEnabled()) {
        statusMessage_ = "Failed to start AP";
        state_ = OtaState::ERROR;
        return false;
    }

    state_ = OtaState::WEB_ACTIVE;
    displayIpAddress_ = WiFi.softAPIP().toString();
    if (!password.isEmpty()) {
        statusMessage_ = password;
    } else {
        statusMessage_ = "(none)";
    }

    setupWebServer();
    webServer_.begin();
    Serial.println("[OTA-LOG] Web OTA update server started.");
    return true;
}

void OtaManager::setupWebServer() {
    webServer_.on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
        String path = OTA_HTML_PAGE_PATH;
        if (SdCardManager::exists(path.c_str())) {
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
        
        String tempPath = String(Firmware::FIRMWARE_DIR_PATH) + "/web_upload.bin";
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
    String tempPath = String(Firmware::FIRMWARE_DIR_PATH) + "/web_upload.bin";
    String md5 = FirmwareUtils::calculateFileMD5(SD, tempPath);
    if (md5.isEmpty() || (currentFirmware_.isValid && md5.equalsIgnoreCase(currentFirmware_.checksum_md5))) {
        SD.remove(tempPath);
        statusMessage_ = md5.isEmpty() ? "MD5 Calc Failed" : "Already up-to-date";
        state_ = OtaState::ERROR;
        request->send(200, "text/plain", statusMessage_);
        return;
    }
    
    statusMessage_ = "Flashing...";
    File updateBin = SD.open(tempPath, FILE_READ);
    if (!updateBin) {
        SD.remove(tempPath);
        state_ = OtaState::ERROR;
        request->send(500, "text/plain", "Cannot reopen temp file for flashing.");
        return;
    }
    
    // Set up onProgress callback for the flashing part
    Update.onProgress([this](size_t uploaded, size_t total){
        progress_.receivedBytes = uploaded;
        progress_.totalBytes = total;
    });

    Update.setMD5(md5.c_str());
    if (Update.begin(updateBin.size()) && Update.writeStream(updateBin) == updateBin.size() && Update.end()) {
        updateBin.close();
        
        FirmwareInfo newFw;
        snprintf(newFw.version, FW_VERSION_MAX_LEN, "web-%s", md5.substring(0, 8).c_str());
        snprintf(newFw.build_date, FW_BUILD_DATE_MAX_LEN, "%s %s", __DATE__, __TIME__);
        snprintf(newFw.binary_filename, FW_BINARY_FILENAME_MAX_LEN, "%s.bin", newFw.version);
        strcpy(newFw.checksum_md5, md5.c_str());
        
        String newBinPath = String(Firmware::FIRMWARE_DIR_PATH) + "/" + newFw.binary_filename;
        String newKfwPath = newBinPath;
        newKfwPath.replace(".bin", Firmware::METADATA_EXTENSION);

        SD.rename(tempPath, newBinPath);
        FirmwareUtils::saveMetadataFile(newKfwPath, newFw);
        saveCurrentFirmware(newFw);

        state_ = OtaState::SUCCESS;
        progress_.receivedBytes = progress_.totalBytes;
        request->send(200, "text/plain", "Success"); // Send success message to browser
        
        scheduleReboot(); // <-- Use new function

    } else {
        updateBin.close();
        SD.remove(tempPath);
        state_ = OtaState::ERROR;
        statusMessage_ = "Flash Failed. Error #" + String(Update.getError());
        request->send(500, "text/plain", statusMessage_);
    }
}