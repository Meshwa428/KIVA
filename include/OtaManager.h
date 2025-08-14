#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <vector>
#include "Firmware.h"
#include <ESPAsyncWebServer.h>
#include <SD.h> // Include for File object

// Forward declarations
class App;
class WifiManager;

enum class OtaState {
    IDLE,
    WEB_ACTIVE,
    BASIC_ACTIVE,
    SD_UPDATING,
    ERROR,
    SUCCESS,
    FLASHING // <-- NEW GENERIC FLASHING STATE
};

// --- Shared Progress Structure ---
struct OtaProgress {
    size_t totalBytes = 0;
    size_t receivedBytes = 0;
};


class OtaManager {
public:
    OtaManager();
    void setup(App* app, WifiManager* wifiManager);
    void loop();

    // --- State Control ---
    bool startWebUpdate();
    void startBasicOta();
    void startSdUpdate(const FirmwareInfo& fwInfo);
    void stop();
    void scanSdForFirmware();

    // --- State Getters ---
    OtaState getState() const;
    String getStatusMessage() const;
    String getDisplayIpAddress() const;
    const std::vector<FirmwareInfo>& getAvailableFirmwares() const;
    const OtaProgress& getProgress() const;

private:
    void loadCurrentFirmware();
    void saveCurrentFirmware(const FirmwareInfo& info);
    void resetState();
    void enterTerminalState();
    void loopFlashing(); // <-- NEW FUNCTION FOR CHUNKED WRITING

    // --- Basic OTA ---
    void setupArduinoOta();

    // --- Web OTA ---
    void setupWebServer();
    void onUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
    void onUpdateEnd(AsyncWebServerRequest *request);
    void onRoot(AsyncWebServerRequest *request);

    const char* getEffectiveOtaPassword();

    App* app_;
    WifiManager* wifiManager_;
    AsyncWebServer webServer_;
    
    OtaState state_;
    String statusMessage_;
    String displayIpAddress_;
    
    OtaProgress progress_;
    
    FirmwareInfo currentFirmware_;
    std::vector<FirmwareInfo> availableSdFirmwares_;

    // OTA process state
    File uploadFile_;
    FirmwareInfo pendingFwInfo_; // Generic for SD or Web
    bool uploadError_;
};

#endif // OTA_MANAGER_H