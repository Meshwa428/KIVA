#include "wifi_manager.h"
#include "sd_card_manager.h"
#include <Arduino.h>
#include <WiFi.h>
#include <cstring>
#include <algorithm>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"

// This must be an extern to the global in KivaMain.ino
extern bool wifiHardwareEnabled;

// Static variables for this file
static WifiConnectionStatus currentAttemptStatus = WIFI_STATUS_IDLE;
static unsigned long connectionStartTime = 0;
static char wifiStatusString[50] = "Wi-Fi Off";

extern KnownWifiNetwork knownNetworksList[MAX_KNOWN_WIFI_NETWORKS];
extern int knownNetworksCount;




void setWifiHardwareState(bool enable, wifi_mode_t modeToSetOnEnable /* = WIFI_MODE_STA */) {
    if (enable) {
        wifi_mode_t currentIdfMode = WIFI_MODE_NULL;
        esp_err_t idfModeGetResult = esp_wifi_get_mode(&currentIdfMode);
        bool isIdfStackCurrentlyActive = (idfModeGetResult == ESP_OK && currentIdfMode != WIFI_MODE_NULL);

        bool needsTeardown = false;
        if (wifiHardwareEnabled || isIdfStackCurrentlyActive) {
            if (isIdfStackCurrentlyActive) {
                if (currentIdfMode != modeToSetOnEnable) {
                    needsTeardown = true;
                    Serial.printf("Wi-Fi Manager: Teardown for mode switch (current: %d, target: %d).\n", currentIdfMode, modeToSetOnEnable);
                } else if (!wifiHardwareEnabled) {
                     needsTeardown = true;
                     Serial.printf("Wi-Fi Manager: Teardown for inconsistent state (Kiva OFF, IDF ON mode %d, target %d).\n", currentIdfMode, modeToSetOnEnable);
                }
            } else { // IDF is NOT active, but Kiva's wifiHardwareEnabled flag was true.
                needsTeardown = true;
                Serial.printf("Wi-Fi Manager: Teardown for inconsistent state (Kiva ON, IDF OFF, target %d).\n", modeToSetOnEnable);
            }
        }


        if (needsTeardown) {
            Serial.println("Wi-Fi Manager: Performing Wi-Fi teardown...");
            if (WiFi.isConnected()) { // Specifically for STA mode activity
                Serial.println("Wi-Fi Manager: Arduino WiFi.disconnect(true,true) for STA.");
                WiFi.disconnect(true, true); // true to clear credentials from SDK
                delay(100);
            }
            // Check if AP was active before stopping. WiFi.softAPIP() != 0 checks if AP was configured.
            // currentIdfMode check ensures we only try to disconnect AP if it was indeed in AP or APSTA mode.
            if ((currentIdfMode == WIFI_MODE_AP || currentIdfMode == WIFI_MODE_APSTA) && WiFi.softAPIP() != IPAddress(0,0,0,0)) {
                Serial.println("Wi-Fi Manager: Arduino WiFi.softAPdisconnect(true) for AP.");
                if (!WiFi.softAPdisconnect(true)) { // true to deinit AP
                     Serial.println("Wi-Fi Manager: WiFi.softAPdisconnect(true) returned false.");
                }
                delay(100);
            }

            Serial.println("Wi-Fi Manager: Explicitly calling esp_wifi_stop().");
            esp_err_t stop_err = esp_wifi_stop();
            if (stop_err != ESP_OK && stop_err != ESP_ERR_WIFI_NOT_STARTED && stop_err != ESP_ERR_WIFI_NOT_INIT) {
                Serial.printf("Wi-Fi Manager: esp_wifi_stop() warning: 0x%x (%s)\n", stop_err, esp_err_to_name(stop_err));
            } else {
                 Serial.println("Wi-Fi Manager: esp_wifi_stop() successful or Wi-Fi was not started/initialized.");
            }
            delay(100);

            Serial.println("Wi-Fi Manager: Explicitly calling esp_wifi_deinit().");
            esp_err_t deinit_err = esp_wifi_deinit();
            if (deinit_err != ESP_OK && deinit_err != ESP_ERR_WIFI_NOT_INIT) {
                Serial.printf("Wi-Fi Manager: esp_wifi_deinit() warning: 0x%x (%s)\n", deinit_err, esp_err_to_name(deinit_err));
            } else {
                Serial.println("Wi-Fi Manager: esp_wifi_deinit() successful or Wi-Fi was not initialized.");
            }
            delay(100);

            // Event loop is NOT deleted.

            wifiHardwareEnabled = false; // Kiva's flag
            currentConnectedSsid = "";
            foundWifiNetworksCount = 0;
            wifiIsScanning = false;
            currentAttemptStatus = WIFI_STATUS_IDLE;
            webOtaActive = false; // Reset OTA flags as Wi-Fi state is changing significantly
            basicOtaStarted = false;
            strncpy(wifiStatusString, "Wi-Fi Off (Post-Teardown)", sizeof(wifiStatusString)-1);
            wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
            Serial.println("Wi-Fi Manager: Teardown complete. Delaying before re-init...");
            delay(200);
        } else if (wifiHardwareEnabled && isIdfStackCurrentlyActive && currentIdfMode == modeToSetOnEnable) {
            Serial.printf("Wi-Fi Manager: Wi-Fi already enabled in target mode %d. No action needed.\n", modeToSetOnEnable);
            return; 
        } else {
            Serial.println("Wi-Fi Manager: Wi-Fi stack appears to be fully inactive. Proceeding to initialization.");
        }


        Serial.println("Wi-Fi Manager: Initializing Wi-Fi stack...");
        // Ensure default event loop exists (safe to call even if already created)
        esp_err_t event_loop_create_err = esp_event_loop_create_default();
        if (event_loop_create_err != ESP_OK && event_loop_create_err != ESP_ERR_INVALID_STATE ) {
            Serial.printf("Wi-Fi Manager: esp_event_loop_create_default() FAILED: 0x%x (%s)\n", event_loop_create_err, esp_err_to_name(event_loop_create_err));
            strncpy(wifiStatusString, "Wi-Fi Enable Fail (EvtLoop)", sizeof(wifiStatusString)-1); wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
            wifiHardwareEnabled = false;
            return;
        }
        Serial.println("Wi-Fi Manager: Default event loop verified/created.");

        // esp_netif_init() is called by Arduino's WiFi library initialization if needed.

        wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
        esp_err_t err_init = esp_wifi_init(&cfg);
        if (err_init != ESP_OK) {
            Serial.printf("Wi-Fi Manager: esp_wifi_init() FAILED: 0x%x (%s).\n", err_init, esp_err_to_name(err_init));
            strncpy(wifiStatusString, "Wi-Fi Enable Fail (Init)", sizeof(wifiStatusString)-1); wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
            wifiHardwareEnabled = false;
            return;
        }

        esp_err_t err_storage = esp_wifi_set_storage(WIFI_STORAGE_RAM);
        if (err_storage != ESP_OK) {
            Serial.printf("Wi-Fi Manager: esp_wifi_set_storage(RAM) failed: 0x%x (%s) (non-critical).\n", err_storage, esp_err_to_name(err_storage));
        }
        
        // Use Arduino WiFi.mode() to set mode, create netifs, and start WiFi.
        Serial.printf("Wi-Fi Manager: Calling Arduino WiFi.mode(%s) to set mode, manage netifs, and start.\n",
                      modeToSetOnEnable == WIFI_MODE_STA ? "STA" :
                      modeToSetOnEnable == WIFI_MODE_AP  ? "AP"  :
                      modeToSetOnEnable == WIFI_MODE_APSTA ? "APSTA" : "NULL (OFF)");

        if (!WiFi.mode(modeToSetOnEnable)) {
            Serial.printf("Wi-Fi Manager: Arduino WiFi.mode(%d) FAILED. Critical Failure.\n", modeToSetOnEnable);
            esp_wifi_deinit(); 
            strncpy(wifiStatusString, "Wi-Fi Enable Fail (Mode)", sizeof(wifiStatusString)-1); wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
            wifiHardwareEnabled = false;
            return;
        }
        // WiFi.mode() internally calls esp_wifi_set_mode and esp_wifi_start.
        // It also handles netif creation/destruction.
        delay(300); // Allow time for mode switch and netif setup to stabilize.

        wifi_mode_t finalIdfModeCheck; // Verify mode with IDF
        if(esp_wifi_get_mode(&finalIdfModeCheck) == ESP_OK && finalIdfModeCheck == modeToSetOnEnable){
            Serial.printf("Wi-Fi Manager: Confirmed IDF mode is %d after Arduino WiFi.mode().\n", finalIdfModeCheck);
        } else {
            Serial.printf("Wi-Fi Manager: CRITICAL - IDF mode is %d, expected %d after Arduino WiFi.mode(). Forcing failure.\n", finalIdfModeCheck, modeToSetOnEnable);
            esp_wifi_stop(); 
            esp_wifi_deinit();
            strncpy(wifiStatusString, "Wi-Fi Enable Fail (Verify)", sizeof(wifiStatusString)-1); wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
            wifiHardwareEnabled = false;
            return;
        }

        if (modeToSetOnEnable == WIFI_MODE_STA || modeToSetOnEnable == WIFI_MODE_APSTA) {
            if(!WiFi.setHostname(DEVICE_HOSTNAME)){ 
                Serial.println("Wi-Fi Manager: Warning - WiFi.setHostname failed.");
            }
            WiFi.persistent(false); 
            WiFi.setAutoReconnect(false); 
        }

        if (modeToSetOnEnable == WIFI_MODE_STA) strncpy(wifiStatusString, "Wi-Fi Idle (STA)", sizeof(wifiStatusString)-1);
        else if (modeToSetOnEnable == WIFI_MODE_AP) strncpy(wifiStatusString, "AP Mode Active", sizeof(wifiStatusString)-1); // Set appropriate status for AP
        else if (modeToSetOnEnable == WIFI_MODE_APSTA) strncpy(wifiStatusString, "Wi-Fi Idle (APSTA)", sizeof(wifiStatusString)-1);
        else strncpy(wifiStatusString, "Wi-Fi Ready", sizeof(wifiStatusString)-1); // Generic, should be one of above
        wifiStatusString[sizeof(wifiStatusString)-1] = '\0';

        wifiHardwareEnabled = true;
        currentAttemptStatus = WIFI_STATUS_IDLE;
        Serial.printf("Wi-Fi Manager: Wi-Fi enabled successfully, target mode %d.\n", modeToSetOnEnable);

    } else { // enable == false
        wifi_mode_t currentIdfModeForDisable = WIFI_MODE_NULL;
        esp_err_t modeGetErrDisable = esp_wifi_get_mode(&currentIdfModeForDisable);
        bool isIdfStackActiveForDisable = (modeGetErrDisable == ESP_OK && currentIdfModeForDisable != WIFI_MODE_NULL);

        if (!wifiHardwareEnabled && !isIdfStackActiveForDisable) { // Already off
            strncpy(wifiStatusString, "Wi-Fi Off", sizeof(wifiStatusString)-1); wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
            return;
        }
        Serial.println("Wi-Fi Manager: Disabling Wi-Fi hardware (Explicit OFF call)...");
        
        // Stop Kiva-specific activities first
        if (wifiIsScanning) {
            wifiIsScanning = false;
            WiFi.scanDelete(); 
            Serial.println("Wi-Fi Manager (Disable): Kiva scan stopped.");
        }
        webOtaActive = false; // Stop any OTA server logic that might try to use Wi-Fi
        basicOtaStarted = false;

        // Use WiFi.mode(WIFI_OFF) for a cleaner shutdown via Arduino API
        // This should handle stopping, deinit, and netif destruction.
        Serial.println("Wi-Fi Manager (Disable): Calling WiFi.mode(WIFI_OFF).");
        if (!WiFi.mode(WIFI_OFF)) { // WIFI_OFF is equivalent to WIFI_MODE_NULL
            Serial.println("Wi-Fi Manager (Disable): WiFi.mode(WIFI_OFF) returned false. Attempting IDF calls.");
            // Fallback to manual IDF calls if WiFi.mode(WIFI_OFF) fails
            esp_wifi_disconnect(); // Disconnect if STA was connected
            delay(100);
            esp_wifi_stop();
            delay(100);
            esp_wifi_deinit();
        } else {
             Serial.println("Wi-Fi Manager (Disable): WiFi.mode(WIFI_OFF) successful.");
        }
        delay(100);

        // Default event loop should NOT be deleted.

        wifiHardwareEnabled = false;
        currentConnectedSsid = "";
        foundWifiNetworksCount = 0;
        currentAttemptStatus = WIFI_STATUS_IDLE;
        strncpy(wifiStatusString, "Wi-Fi Off", sizeof(wifiStatusString)-1); wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
        Serial.println("Wi-Fi Manager: Wi-Fi fully disabled.");
    }
}



void updateWifiStatusOnDisconnect() {
  currentAttemptStatus = WIFI_STATUS_IDLE;
  strncpy(wifiStatusString, "Disconnected", sizeof(wifiStatusString) - 1);
  wifiStatusString[sizeof(wifiStatusString) - 1] = '\0';
  currentConnectedSsid = "";
}


bool loadKnownWifiNetworks() {
  if (!isSdCardAvailable()) {
    Serial.println("WiFi Creds Load: SD card not available.");
    knownNetworksCount = 0;
    return false;
  }
  // Serial.println("WiFi Creds Load: Attempting to load known networks from SD."); // Reduce verbosity
  String fileContent = readSdFile(KNOWN_WIFI_FILE_PATH);
  if (fileContent.length() == 0) {
    // Serial.println("WiFi Creds Load: Known networks file is empty or could not be read."); // Reduce verbosity
    knownNetworksCount = 0;
    return true;
  }
  knownNetworksCount = 0;
  int currentIndex = 0;
  int lineStart = 0;
  int linesProcessed = 0;
  while (currentIndex < fileContent.length() && knownNetworksCount < MAX_KNOWN_WIFI_NETWORKS) {
    int ssidEnd = fileContent.indexOf(';', lineStart);
    if (ssidEnd == -1) break;
    int passEnd = fileContent.indexOf(';', ssidEnd + 1);
    if (passEnd == -1) break;
    int failCountEnd = fileContent.indexOf('\n', passEnd + 1);
    if (failCountEnd == -1) failCountEnd = fileContent.length();

    String ssidStr = fileContent.substring(lineStart, ssidEnd);
    String passStr = fileContent.substring(ssidEnd + 1, passEnd);
    String failStr = fileContent.substring(passEnd + 1, failCountEnd);
    failStr.trim();
    if (ssidStr.length() == 0 || ssidStr.length() > 32) {
      lineStart = failCountEnd + 1;
      linesProcessed++;
      if (lineStart >= fileContent.length()) break;
      continue;
    }
    strncpy(knownNetworksList[knownNetworksCount].ssid, ssidStr.c_str(), sizeof(knownNetworksList[knownNetworksCount].ssid) - 1);
    knownNetworksList[knownNetworksCount].ssid[sizeof(knownNetworksList[knownNetworksCount].ssid) - 1] = '\0';
    strncpy(knownNetworksList[knownNetworksCount].password, passStr.c_str(), sizeof(knownNetworksList[knownNetworksCount].password) - 1);
    knownNetworksList[knownNetworksCount].password[sizeof(knownNetworksList[knownNetworksCount].password) - 1] = '\0';
    knownNetworksList[knownNetworksCount].failCount = failStr.toInt();
    if (knownNetworksList[knownNetworksCount].failCount == 0 && failStr != "0") knownNetworksList[knownNetworksCount].failCount = 0;
    knownNetworksCount++;
    linesProcessed++;
    lineStart = failCountEnd + 1;
    if (lineStart >= fileContent.length()) break;
  }
  Serial.printf("WiFi Creds Load: Loaded %d networks.\n", knownNetworksCount);
  return true;
}

bool saveKnownWifiNetworks() {
  if (!isSdCardAvailable()) {
    Serial.println("WiFi Creds Save: SD card not available. CANNOT SAVE.");
    return false;
  }
  String fileContent = "";
  for (int i = 0; i < knownNetworksCount; i++) {
    if (strlen(knownNetworksList[i].ssid) == 0) continue;
    fileContent += String(knownNetworksList[i].ssid) + ";";
    fileContent += String(knownNetworksList[i].password) + ";";
    fileContent += String(knownNetworksList[i].failCount) + "\n";
  }
  if (writeSdFile(KNOWN_WIFI_FILE_PATH, fileContent.c_str())) {
    // Serial.printf("WiFi Creds Save: Saved %d networks.\n", knownNetworksCount); // Reduce verbosity
    return true;
  } else {
    Serial.println("WiFi Creds Save: FAILED to save networks file.");
    return false;
  }
}

KnownWifiNetwork* findKnownNetwork(const char* ssid) {
  for (int i = 0; i < knownNetworksCount; i++) {
    if (strcmp(knownNetworksList[i].ssid, ssid) == 0) return &knownNetworksList[i];
  }
  return nullptr;
}

bool addOrUpdateKnownNetwork(const char* ssid, const char* password, bool resetFailCountOnUpdate) {
  if (strlen(ssid) == 0) return false;
  KnownWifiNetwork* existingNetwork = findKnownNetwork(ssid);
  bool changed = false;
  if (existingNetwork) {
    if (strcmp(existingNetwork->password, password) != 0) {
      strncpy(existingNetwork->password, password, sizeof(existingNetwork->password) - 1);
      existingNetwork->password[sizeof(existingNetwork->password) - 1] = '\0';
      changed = true;
    }
    if (resetFailCountOnUpdate && existingNetwork->failCount != 0) {
      existingNetwork->failCount = 0;
      changed = true;
    }
  } else {
    if (knownNetworksCount < MAX_KNOWN_WIFI_NETWORKS) {
      strncpy(knownNetworksList[knownNetworksCount].ssid, ssid, sizeof(knownNetworksList[knownNetworksCount].ssid) - 1);
      knownNetworksList[knownNetworksCount].ssid[sizeof(knownNetworksList[knownNetworksCount].ssid) - 1] = '\0';
      strncpy(knownNetworksList[knownNetworksCount].password, password, sizeof(knownNetworksList[knownNetworksCount].password) - 1);
      knownNetworksList[knownNetworksCount].password[sizeof(knownNetworksList[knownNetworksCount].password) - 1] = '\0';
      knownNetworksList[knownNetworksCount].failCount = 0;
      knownNetworksCount++;
      changed = true;
    } else {
      Serial.println("WiFi Creds Add/Update: Max known networks reached.");
      return false;
    }
  }
  if (changed) return saveKnownWifiNetworks();
  return true;
}

bool incrementSsidFailCount(const char* ssid) {
  if (strlen(ssid) == 0) return false;
  KnownWifiNetwork* network = findKnownNetwork(ssid);
  if (network) {
    network->failCount++;
    Serial.printf("WiFi Creds FailCount: Incremented for %s to %d.\n", ssid, network->failCount);
    return saveKnownWifiNetworks();
  }
  return false;
}

bool resetSsidFailCount(const char* ssid) {
  if (strlen(ssid) == 0) return false;
  KnownWifiNetwork* network = findKnownNetwork(ssid);
  if (network) {
    if (network->failCount != 0) {
      network->failCount = 0;
      return saveKnownWifiNetworks();
    }
    return true;
  }
  return false;
}


void setupWifi() {
  setWifiHardwareState(false);
  if (isSdCardAvailable()) {
    loadKnownWifiNetworks();
  } else {
    Serial.println("Wi-Fi Manager: SD Card not available, cannot load known Wi-Fi networks.");
    knownNetworksCount = 0;
  }
  Serial.println("Wi-Fi Manager: Setup complete. Wi-Fi hardware initially OFF.");
}

int initiateAsyncWifiScan() {
  if (!wifiHardwareEnabled) {
    Serial.println("Wi-Fi Manager: Cannot scan, Wi-Fi hardware is disabled.");
    strncpy(wifiStatusString, "Wi-Fi Off", sizeof(wifiStatusString) - 1); // Set internal status
    wifiStatusString[sizeof(wifiStatusString) - 1] = '\0';
    foundWifiNetworksCount = 0;
    wifiIsScanning = false;
    currentAttemptStatus = WIFI_STATUS_IDLE;
    return -3;
  }
  Serial.println("Wi-Fi Manager: Initiating async scan...");
  WiFi.scanDelete();
  delay(50);
  int scanResult = WiFi.scanNetworks(true, true);
  if (scanResult == WIFI_SCAN_FAILED) {
    Serial.println("Wi-Fi Manager: Scan initiation failed.");
    currentAttemptStatus = KIVA_WIFI_SCAN_FAILED;
    strncpy(wifiStatusString, "Scan Failed", sizeof(wifiStatusString) - 1); // Set internal status
    wifiIsScanning = false;
  } else if (scanResult == WIFI_SCAN_RUNNING) {
    Serial.println("Wi-Fi Manager: Scan started successfully, running in background.");
    currentAttemptStatus = KIVA_WIFI_SCAN_RUNNING;
    strncpy(wifiStatusString, "Scanning...", sizeof(wifiStatusString) - 1); // Set internal status
    wifiIsScanning = true;
  } else { 
    Serial.printf("Wi-Fi Manager: scanNetworks (async) returned %d. Processing results.\n", scanResult);
    wifiIsScanning = false; 
    // wifiStatusString will be updated by checkAndRetrieveWifiScanResults()
    return checkAndRetrieveWifiScanResults();
  }
  wifiStatusString[sizeof(wifiStatusString) - 1] = '\0';
  return scanResult;
}

int checkAndRetrieveWifiScanResults() {
  int scanStatus = WiFi.scanComplete();
  if (scanStatus == WIFI_SCAN_FAILED) {
    Serial.println("Wi-Fi Manager: WiFi.scanComplete() reported scan FAILED.");
    foundWifiNetworksCount = 0;
    currentAttemptStatus = KIVA_WIFI_SCAN_FAILED;
    strncpy(wifiStatusString, "Scan Failed", sizeof(wifiStatusString) - 1);
    wifiStatusString[sizeof(wifiStatusString) - 1] = '\0';
    wifiIsScanning = false;
    WiFi.scanDelete();
    return KIVA_WIFI_SCAN_FAILED;
  } else if (scanStatus == WIFI_SCAN_RUNNING) {
    currentAttemptStatus = KIVA_WIFI_SCAN_RUNNING;  // Still running
    return KIVA_WIFI_SCAN_RUNNING;
  } else if (scanStatus >= 0) {  // Scan finished, scanStatus = number of networks
    Serial.printf("Wi-Fi Manager: Scan done, %d networks found by WiFi.scanComplete().\n", scanStatus);
    wifiIsScanning = false;
    WifiNetwork tempScannedNetworks[MAX_WIFI_NETWORKS];
    int tempCount = 0;
    if (scanStatus > 0) {
      for (int i = 0; i < scanStatus && tempCount < MAX_WIFI_NETWORKS; ++i) {
        String ssidStr = WiFi.SSID(i);
        ssidStr.trim();
        if (ssidStr.length() == 0 || ssidStr.length() > 32) continue;
        strncpy(tempScannedNetworks[tempCount].ssid, ssidStr.c_str(), 32);
        tempScannedNetworks[tempCount].ssid[32] = '\0';
        tempScannedNetworks[tempCount].rssi = WiFi.RSSI(i);
        tempScannedNetworks[tempCount].isSecure = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
        tempCount++;
      }
    }
    std::sort(tempScannedNetworks, tempScannedNetworks + tempCount, [](const WifiNetwork& a, const WifiNetwork& b) {
      return a.rssi > b.rssi;
    });
    foundWifiNetworksCount = 0;
    int connectedNetworkOriginalIndexInTemp = -1;
    if (currentConnectedSsid.length() > 0) {
      for (int i = 0; i < tempCount; ++i) {
        if (currentConnectedSsid.equals(tempScannedNetworks[i].ssid)) {
          if (foundWifiNetworksCount < MAX_WIFI_NETWORKS) {
            scannedNetworks[foundWifiNetworksCount++] = tempScannedNetworks[i];
            connectedNetworkOriginalIndexInTemp = i;
          }
          break;
        }
      }
    }
    for (int i = 0; i < tempCount; ++i) {
      if (i == connectedNetworkOriginalIndexInTemp) continue;
      if (foundWifiNetworksCount < MAX_WIFI_NETWORKS) {
        scannedNetworks[foundWifiNetworksCount++] = tempScannedNetworks[i];
      } else break;
    }
    if (foundWifiNetworksCount == 0) strncpy(wifiStatusString, "No Networks", sizeof(wifiStatusString) - 1);
    else snprintf(wifiStatusString, sizeof(wifiStatusString) - 1, "%d Networks", foundWifiNetworksCount);
    wifiStatusString[sizeof(wifiStatusString) - 1] = '\0';
    currentAttemptStatus = WIFI_STATUS_IDLE;
    WiFi.scanDelete();
    return foundWifiNetworksCount;
  }
  Serial.printf("Wi-Fi Manager: WiFi.scanComplete() returned unhandled: %d\n", scanStatus);
  currentAttemptStatus = WIFI_STATUS_IDLE;
  strncpy(wifiStatusString, "Scan Unknown", sizeof(wifiStatusString) - 1);
  wifiStatusString[sizeof(wifiStatusString) - 1] = '\0';
  wifiIsScanning = false;
  WiFi.scanDelete();
  return scanStatus;  // Propagate unusual status
}


wl_status_t getCurrentWifiStatus() {
  return WiFi.status();
}

String getCurrentSsid() {
  if (WiFi.status() == WL_CONNECTED) {
    currentConnectedSsid = WiFi.SSID();  // Update our global
    return currentConnectedSsid;
  }
  return "";  // Return empty if not connected
}

void attemptDirectWifiConnection(const char* ssid) {
  if (!wifiHardwareEnabled) {
    Serial.println("Wi-Fi Manager: Attempting direct connection but Wi-Fi is off. Enabling first.");
    setWifiHardwareState(true);
    delay(200);                  // Allow time for hardware to initialize
    if (!wifiHardwareEnabled) {  // Double-check if enabling failed
      Serial.println("Wi-Fi Manager: Failed to enable Wi-Fi hardware. Aborting connection attempt.");
      currentAttemptStatus = WIFI_FAILED_OTHER;  // Or a more specific "hardware error" status
      strncpy(wifiStatusString, "Wi-Fi HW Error", sizeof(wifiStatusString) - 1);
      wifiStatusString[sizeof(wifiStatusString) - 1] = '\0';
      return;
    }
  }

  Serial.printf("Wi-Fi Manager: Attempting OPEN connection to: %s\n", ssid);
  currentConnectedSsid = "";  // Clear any old connected SSID

  // Ensure STA mode and disconnect if already connected or in a weird state
  if (WiFi.getMode() != WIFI_STA) {
    Serial.println("Wi-Fi Manager: Mode not STA, setting to STA.");
    WiFi.mode(WIFI_STA);  // This might do some internal init
    delay(100);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("Wi-Fi Manager: Was connected to %s. Disconnecting...\n", WiFi.SSID().c_str());
    WiFi.disconnect(true);  // true to erase SDK config
    delay(200);             // Give disconnect time
  } else {
    // Good practice to call disconnect anyway to clear SDK state if begin fails later
    WiFi.disconnect(true);
    delay(100);
  }

  // Configure for DHCP
  IPAddress emptyIP(0, 0, 0, 0);
  WiFi.config(emptyIP, emptyIP, emptyIP, emptyIP);  // Force DHCP, clear DNS
  delay(50);

  Serial.printf("Wi-Fi Manager: Calling WiFi.begin(\"%s\")\n", ssid);
  WiFi.begin(ssid);  // For open networks
  connectionStartTime = millis();
  currentAttemptStatus = WIFI_CONNECTING_IN_PROGRESS;
  strncpy(wifiStatusString, "Connecting...", sizeof(wifiStatusString) - 1);
  wifiStatusString[sizeof(wifiStatusString) - 1] = '\0';

  // For an open network, password is "", reset fail count for this attempt
  addOrUpdateKnownNetwork(ssid, "", true);
}

void attemptWpaWifiConnection(const char* ssid, const char* password) {
  if (!wifiHardwareEnabled) {
    Serial.println("Wi-Fi Manager: Attempting WPA connection but Wi-Fi is off. Enabling first.");
    setWifiHardwareState(true);
    delay(200);                  // Allow time for hardware to initialize
    if (!wifiHardwareEnabled) {  // Double-check if enabling failed
      Serial.println("Wi-Fi Manager: Failed to enable Wi-Fi hardware. Aborting connection attempt.");
      currentAttemptStatus = WIFI_FAILED_OTHER;
      strncpy(wifiStatusString, "Wi-Fi HW Error", sizeof(wifiStatusString) - 1);
      wifiStatusString[sizeof(wifiStatusString) - 1] = '\0';
      return;
    }
  }

  Serial.printf("Wi-Fi Manager: Attempting SECURE connection to: %s\n", ssid);
  currentConnectedSsid = "";

  if (WiFi.getMode() != WIFI_STA) {
    Serial.println("Wi-Fi Manager: Mode not STA, setting to STA.");
    WiFi.mode(WIFI_STA);
    delay(100);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("Wi-Fi Manager: Was connected to %s. Disconnecting...\n", WiFi.SSID().c_str());
    WiFi.disconnect(true);
    delay(200);
  } else {
    WiFi.disconnect(true);
    delay(100);
  }

  IPAddress emptyIP(0, 0, 0, 0);
  WiFi.config(emptyIP, emptyIP, emptyIP, emptyIP);
  delay(50);

  Serial.printf("Wi-Fi Manager: Calling WiFi.begin(\"%s\", PW)\n", ssid);
  WiFi.begin(ssid, password);
  connectionStartTime = millis();
  currentAttemptStatus = WIFI_CONNECTING_IN_PROGRESS;
  strncpy(wifiStatusString, "Connecting...", sizeof(wifiStatusString) - 1);
  wifiStatusString[sizeof(wifiStatusString) - 1] = '\0';
  // addOrUpdateKnownNetwork is handled by the calling UI logic (menu_logic.cpp)
  // after password input or when using a known password. Resetting fail count
  // is also handled there or by a successful connection.
}

WifiConnectionStatus checkWifiConnectionProgress() {
  if (currentAttemptStatus != WIFI_CONNECTING_IN_PROGRESS) {
    return currentAttemptStatus;
  }

  wl_status_t status = WiFi.status();
  const char* ssidOfCurrentAttempt = currentSsidToConnect;

  if (status == WL_CONNECTED) {
    Serial.println("Wi-Fi Manager: Connected successfully!");
    currentAttemptStatus = WIFI_CONNECTED_SUCCESS;
    currentConnectedSsid = WiFi.SSID();
    snprintf(wifiStatusString, sizeof(wifiStatusString) - 1, "Connected: %s", currentConnectedSsid.c_str());
    if (strlen(ssidOfCurrentAttempt) > 0) {
      resetSsidFailCount(ssidOfCurrentAttempt);
    }
  } else if (status == WL_NO_SSID_AVAIL) {
    Serial.printf("Wi-Fi Manager: Connection Failed for '%s' - SSID not available (WL_NO_SSID_AVAIL).\n", ssidOfCurrentAttempt);
    currentAttemptStatus = WIFI_FAILED_NO_SSID_AVAIL;
    strncpy(wifiStatusString, "SSID Not Found", sizeof(wifiStatusString) - 1);
    currentConnectedSsid = "";
    if (strlen(ssidOfCurrentAttempt) > 0) incrementSsidFailCount(ssidOfCurrentAttempt);
  } else if (status == WL_CONNECT_FAILED) {
    Serial.printf("Wi-Fi Manager: Connection Failed for '%s' - General connect failed (WL_CONNECT_FAILED).\n", ssidOfCurrentAttempt);
    if (selectedNetworkIsSecure && strlen(ssidOfCurrentAttempt) > 0) {
      currentAttemptStatus = WIFI_FAILED_WRONG_PASSWORD;
      strncpy(wifiStatusString, "Auth Error", sizeof(wifiStatusString) - 1);
    } else {
      currentAttemptStatus = WIFI_FAILED_OTHER;
      strncpy(wifiStatusString, "Connect Fail", sizeof(wifiStatusString) - 1);
    }
    currentConnectedSsid = "";
    if (strlen(ssidOfCurrentAttempt) > 0) incrementSsidFailCount(ssidOfCurrentAttempt);
  } else if (millis() - connectionStartTime > WIFI_CONNECTION_TIMEOUT_MS) {  // Moved timeout check higher priority for WL_IDLE/DISCONNECTED
    Serial.printf("Wi-Fi Manager: Connection Timeout for '%s'. Last ESP32 WiFi status was: %d\n", ssidOfCurrentAttempt, status);
    if (selectedNetworkIsSecure && strlen(ssidOfCurrentAttempt) > 0) {
      // If it was a secure network and timed out, it's highly likely an auth issue
      // or the AP is not responding correctly to auth attempts.
      currentAttemptStatus = WIFI_FAILED_WRONG_PASSWORD;  // Prioritize this over generic timeout for secure nets
      strncpy(wifiStatusString, "Auth Error/Timeout", sizeof(wifiStatusString) - 1);
    } else {
      // For open networks or if not secure, it's a general timeout
      currentAttemptStatus = WIFI_FAILED_TIMEOUT;
      strncpy(wifiStatusString, "Timeout", sizeof(wifiStatusString) - 1);
    }
    currentConnectedSsid = "";
    if (strlen(ssidOfCurrentAttempt) > 0) {
      incrementSsidFailCount(ssidOfCurrentAttempt);
    }
    WiFi.disconnect(true);  // Ensure disconnected state after timeout
  } else if (status == WL_IDLE_STATUS || status == WL_DISCONNECTED || status == WL_SCAN_COMPLETED) {
    // Still waiting, status is idle/disconnected but not yet timed out by our custom timer.
    // WiFi.begin() is still working.
    currentAttemptStatus = WIFI_CONNECTING_IN_PROGRESS;  // Keep this state
                                                         // wifiStatusString should still be "Connecting..." - no change needed here.
  }
  // Removed the 'else' block that handled generic timeout for other statuses,
  // as the main timeout block above now covers all scenarios once WIFI_CONNECTION_TIMEOUT_MS is reached.
  // If it's not connected, not failed explicitly, and not timed out, it's still in progress.

  wifiStatusString[sizeof(wifiStatusString) - 1] = '\0';
  return currentAttemptStatus;
}

WifiConnectionStatus getCurrentWifiConnectionStatus() {
  if (!wifiHardwareEnabled) {
    // If hardware is off, report idle, not necessarily the last attempt status
    return WIFI_STATUS_IDLE;
  }
  return currentAttemptStatus;
}

const char* getWifiStatusMessage() {
  if (!wifiHardwareEnabled) {
    return "Wi-Fi Off";
  }
  // For "Scanning...", rely on wifiIsScanning flag checked by caller (e.g. drawWifiSetupScreen)
  // This function primarily returns status related to connection attempts or general state.
  return wifiStatusString;
}