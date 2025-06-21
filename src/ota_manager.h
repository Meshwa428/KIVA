#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include "config.h"
#include "wifi_manager.h"
#include "sd_card_manager.h"
#include "firmware_metadata.h"
#include <WebServer.h> // <--- ENSURE THIS IS PRESENT
#include <ESPmDNS.h>
#include <Update.h>
#include <ArduinoOTA.h>
#include <Stream.h>
#include <WiFi.h>

#define OTA_PROGRESS_PENDING_SD_START 1 // A special progress value
#define OTA_AP_PASSWORD_FILE "/system/ota_ap_passwd.txt"
#define MIN_AP_PASSWORD_LEN 8 // Minimum length for a valid password from file
#define MAX_AP_PASSWORD_LEN 63 // Maximum length (WiFi standard)

// Extern global variables (defined in KivaMain.ino)
extern WebServer otaWebServer;
extern int otaProgress;
extern String otaStatusMessage;
extern bool otaIsUndoingChanges;
extern bool basicOtaStarted;
extern bool webOtaActive;
extern char otaDisplayIpAddress[40];

// Externs related to firmware info (defined in KivaMain.ino)
extern FirmwareInfo currentRunningFirmware;


// Web Updater HTML and favicon (defined in ota_manager.cpp)
extern const char *indexHtml;
extern const char favicon_ico_gz[];
extern const int favicon_ico_gz_len;

// Functions
void setupOtaManager();

// Firmware Info Management
bool loadCurrentFirmwareInfo();
bool saveCurrentFirmwareInfo(const FirmwareInfo& infoToSave); // Overwrites current_fw.json

// Web OTA
bool startWebOtaUpdate();
void handleWebOtaClient();
void stopWebOtaUpdate();

// SD OTA
bool prepareSdFirmwareList(); // Scans FIRMWARE_DIR_PATH, populates availableSdFirmwares
void performSdUpdate(const FirmwareInfo& targetFirmware); // Triggered from FIRMWARE_SD_LIST_MENU

// Basic OTA (IDE)
bool startBasicOtaUpdate();
void handleBasicOta();
void stopBasicOtaUpdate();

void resetOtaState();
void configureArduinoOta();

#endif // OTA_MANAGER_H