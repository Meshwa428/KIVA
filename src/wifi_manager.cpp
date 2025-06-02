#include "wifi_manager.h"
#include "sd_card_manager.h"
#include <Arduino.h>
#include <WiFi.h>
#include <cstring>
#include <algorithm>

// This must be an extern to the global in KivaMain.ino
extern bool wifiHardwareEnabled; 

// Static variables for this file
static WifiConnectionStatus currentAttemptStatus = WIFI_STATUS_IDLE;
static unsigned long connectionStartTime = 0;
const unsigned long WIFI_CONNECTION_TIMEOUT_MS = 15000;
// const unsigned long WIFI_CONNECTION_TIMEOUT_MS = 15000; // Already in .h via config.h or similar
static char wifiStatusString[50] = "Wi-Fi Off";

// Definitions for known networks list (already externed in .h)
// KnownWifiNetwork knownNetworksList[MAX_KNOWN_WIFI_NETWORKS]; // Definition should be in KivaMain.ino
// int knownNetworksCount = 0; // Definition should be in KivaMain.ino
// Corrected: These are global and defined in KivaMain.ino. Accessed via extern.
extern KnownWifiNetwork knownNetworksList[MAX_KNOWN_WIFI_NETWORKS];
extern int knownNetworksCount;


void setWifiHardwareState(bool enable) {
    if (enable && !wifiHardwareEnabled) {
        Serial.println("Wi-Fi Manager: Enabling Wi-Fi hardware.");
        WiFi.mode(WIFI_STA); 
        if (WiFi.setHostname(DEVICE_HOSTNAME)) {
            Serial.printf("Wi-Fi Manager: Hostname set to '%s'\n", DEVICE_HOSTNAME);
        } else {
            Serial.println("Wi-Fi Manager: Failed to set hostname during init.");
        }
        WiFi.persistent(false); 
        WiFi.setAutoReconnect(false); 
        wifiHardwareEnabled = true;
        currentAttemptStatus = WIFI_STATUS_IDLE;
        strncpy(wifiStatusString, "Wi-Fi Idle", sizeof(wifiStatusString)-1);
        wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
    } else if (!enable && wifiHardwareEnabled) {
        Serial.println("Wi-Fi Manager: Disabling Wi-Fi hardware.");
        if (wifiIsScanning) {
             wifiIsScanning = false;
        }
        WiFi.disconnect(true); 
        delay(100); 
        WiFi.mode(WIFI_OFF);   
        wifiHardwareEnabled = false;
        currentConnectedSsid = "";
        foundWifiNetworksCount = 0; 
        currentAttemptStatus = WIFI_STATUS_IDLE;
        strncpy(wifiStatusString, "Wi-Fi Off", sizeof(wifiStatusString)-1);
        wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
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
        Serial.println("WiFi Creds Load: SD card not available. Cannot load known networks.");
        knownNetworksCount = 0;
        return false;
    }
    Serial.println("WiFi Creds Load: Attempting to load known networks from SD.");
    String fileContent = readSdFile(KNOWN_WIFI_FILE_PATH);
    
    if (fileContent.length() == 0) {
        Serial.println("WiFi Creds Load: Known networks file is empty or could not be read.");
        knownNetworksCount = 0;
        return true;
    }

    knownNetworksCount = 0;
    int currentIndex = 0;
    int lineStart = 0;
    int linesProcessed = 0;

    while (currentIndex < fileContent.length() && knownNetworksCount < MAX_KNOWN_WIFI_NETWORKS) {
        int ssidEnd = fileContent.indexOf(';', lineStart);
        if (ssidEnd == -1) {
            Serial.printf("WiFi Creds Load: Malformed line %d (missing SSID terminator) at index %d. Stopping parse.\n", linesProcessed + 1, lineStart);
            break; 
        }

        int passEnd = fileContent.indexOf(';', ssidEnd + 1);
        if (passEnd == -1) {
             Serial.printf("WiFi Creds Load: Malformed line %d (missing Password terminator) after SSID '%s'. Stopping parse.\n", linesProcessed + 1, fileContent.substring(lineStart, ssidEnd).c_str());
            break;
        }

        int failCountEnd = fileContent.indexOf('\n', passEnd + 1);
        if (failCountEnd == -1 && (passEnd +1 < fileContent.length())) { 
             failCountEnd = fileContent.length(); 
             Serial.printf("WiFi Creds Load: Line %d missing newline, assuming end of file.\n", linesProcessed + 1);
        } else if (failCountEnd == -1) {
             failCountEnd = fileContent.length();
        }


        String ssidStr = fileContent.substring(lineStart, ssidEnd);
        String passStr = fileContent.substring(ssidEnd + 1, passEnd);
        String failStr = fileContent.substring(passEnd + 1, failCountEnd);
        failStr.trim();

        if (ssidStr.length() == 0 || ssidStr.length() > 32) {
            Serial.printf("WiFi Creds Load: Invalid SSID length on line %d. Skipping.\n", linesProcessed + 1);
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
        if (failStr.toInt() == 0 && failStr != "0") {
            Serial.printf("WiFi Creds Load: Warning - could not parse fail count '%s' for SSID '%s' on line %d. Defaulting to 0.\n", failStr.c_str(), ssidStr.c_str(), linesProcessed + 1);
            knownNetworksList[knownNetworksCount].failCount = 0;
        }

        knownNetworksCount++;
        linesProcessed++;
        lineStart = failCountEnd + 1;
        if (lineStart >= fileContent.length()) break;
    }
    Serial.printf("WiFi Creds Load: Successfully parsed and loaded %d known networks from %d lines.\n", knownNetworksCount, linesProcessed);
    return true;
}

bool saveKnownWifiNetworks() {
    if (!isSdCardAvailable()) {
        Serial.println("WiFi Creds Save: SD card not available. CANNOT SAVE known networks.");
        return false;
    }
    if (knownNetworksCount == 0) {
        Serial.println("WiFi Creds Save: No known networks in list to save. Ensuring file is empty or deleted.");
        if (writeSdFile(KNOWN_WIFI_FILE_PATH, "")) {
             Serial.println("WiFi Creds Save: Emptied known networks file.");
             return true;
        } else {
             Serial.println("WiFi Creds Save: Failed to empty known networks file.");
             return false;
        }
    }

    String fileContent = "";
    for (int i = 0; i < knownNetworksCount; i++) {
        if (strlen(knownNetworksList[i].ssid) == 0) {
            Serial.printf("WiFi Creds Save: Skipping network at index %d due to empty SSID.\n", i);
            continue;
        }
        fileContent += String(knownNetworksList[i].ssid) + ";";
        fileContent += String(knownNetworksList[i].password) + ";";
        fileContent += String(knownNetworksList[i].failCount) + "\n";
    }

    Serial.printf("WiFi Creds Save: Attempting to save %d known networks to SD.\n", knownNetworksCount);
    if (writeSdFile(KNOWN_WIFI_FILE_PATH, fileContent.c_str())) {
        Serial.printf("WiFi Creds Save: Successfully saved %d known networks.\n", knownNetworksCount);
        return true;
    } else {
        Serial.println("WiFi Creds Save: FAILED to save known networks file.");
        return false;
    }
}

KnownWifiNetwork* findKnownNetwork(const char* ssid) {
    for (int i = 0; i < knownNetworksCount; i++) {
        if (strcmp(knownNetworksList[i].ssid, ssid) == 0) {
            return &knownNetworksList[i];
        }
    }
    return nullptr;
}

bool addOrUpdateKnownNetwork(const char* ssid, const char* password, bool resetFailCountOnUpdate) {
    if (strlen(ssid) == 0) {
        Serial.println("WiFi Creds Add/Update: Attempted to add/update with an empty SSID. Aborting.");
        return false;
    }

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
        if (changed) {
            Serial.printf("WiFi Creds Add/Update: Updated network %s. ResetFail: %d\n", ssid, resetFailCountOnUpdate);
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
            Serial.printf("WiFi Creds Add/Update: Added new network %s.\n", ssid);
        } else {
            Serial.println("WiFi Creds Add/Update: Max known networks reached. Cannot add new one.");
            return false; 
        }
    }

    if (changed) {
        return saveKnownWifiNetworks();
    }
    return true;
}

bool incrementSsidFailCount(const char* ssid) {
    if (strlen(ssid) == 0) return false;
    KnownWifiNetwork* network = findKnownNetwork(ssid);
    if (network) {
        network->failCount++;
        Serial.printf("WiFi Creds FailCount: Incremented fail count for %s to %d.\n", ssid, network->failCount);
        return saveKnownWifiNetworks();
    }
    Serial.printf("WiFi Creds FailCount: Could not find network %s to increment fail count.\n", ssid);
    return false;
}

bool resetSsidFailCount(const char* ssid) {
    if (strlen(ssid) == 0) return false;
    KnownWifiNetwork* network = findKnownNetwork(ssid);
    if (network) {
        if (network->failCount != 0) { 
            network->failCount = 0;
            Serial.printf("WiFi Creds FailCount: Reset fail count for %s.\n", ssid);
            return saveKnownWifiNetworks();
        }
        return true; 
    }
    Serial.printf("WiFi Creds FailCount: Could not find network %s to reset fail count.\n", ssid);
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
        strncpy(wifiStatusString, "Wi-Fi Off", sizeof(wifiStatusString)-1);
        wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
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
        strncpy(wifiStatusString, "Scan Failed", sizeof(wifiStatusString)-1);
        wifiIsScanning = false;
    } else if (scanResult == WIFI_SCAN_RUNNING) { 
        Serial.println("Wi-Fi Manager: Scan started successfully, running in background.");
        currentAttemptStatus = KIVA_WIFI_SCAN_RUNNING;
        strncpy(wifiStatusString, "Scanning...", sizeof(wifiStatusString)-1);
        wifiIsScanning = true;
    } else { 
        Serial.printf("Wi-Fi Manager: scanNetworks call returned unexpected for async: %d\n", scanResult);
        currentAttemptStatus = WIFI_STATUS_IDLE;
        strncpy(wifiStatusString, "Scan Done?", sizeof(wifiStatusString)-1);
        wifiIsScanning = false;
    }
    wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
    return scanResult; 
}

int checkAndRetrieveWifiScanResults() {
    int scanStatus = WiFi.scanComplete(); 

    if (scanStatus == WIFI_SCAN_FAILED) {
        Serial.println("Wi-Fi Manager: WiFi.scanComplete() reported scan FAILED.");
        foundWifiNetworksCount = 0;
        currentAttemptStatus = KIVA_WIFI_SCAN_FAILED;
        strncpy(wifiStatusString, "Scan Failed", sizeof(wifiStatusString)-1);
        wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
        wifiIsScanning = false;
        WiFi.scanDelete();
        return KIVA_WIFI_SCAN_FAILED;
    } else if (scanStatus == WIFI_SCAN_RUNNING) {
        currentAttemptStatus = KIVA_WIFI_SCAN_RUNNING;
        return KIVA_WIFI_SCAN_RUNNING;
    } else if (scanStatus >= 0) {
        Serial.print("Wi-Fi Manager: Async Scan done, ");
        Serial.print(scanStatus); 
        Serial.println(" networks found by WiFi.scanComplete().");
        wifiIsScanning = false;

        WifiNetwork tempScannedNetworks[MAX_WIFI_NETWORKS]; 
        int tempCount = 0;

        if (scanStatus > 0) {
            for (int i = 0; i < scanStatus && tempCount < MAX_WIFI_NETWORKS; ++i) {
                String ssidStr = WiFi.SSID(i);
                ssidStr.trim();
                if (ssidStr.length() == 0 || ssidStr.length() > 32) {
                    continue;
                }
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
            if (i == connectedNetworkOriginalIndexInTemp) {
                continue;
            }
            if (foundWifiNetworksCount < MAX_WIFI_NETWORKS) {
                scannedNetworks[foundWifiNetworksCount++] = tempScannedNetworks[i];
            } else {
                break;
            }
        }

        if (foundWifiNetworksCount == 0) {
             Serial.println("Wi-Fi Manager: No valid networks stored after filtering and sorting.");
             strncpy(wifiStatusString, "No Networks", sizeof(wifiStatusString)-1);
        } else {
             snprintf(wifiStatusString, sizeof(wifiStatusString)-1, "%d Networks", foundWifiNetworksCount);
        }
        wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
        currentAttemptStatus = WIFI_STATUS_IDLE;
        
        WiFi.scanDelete();
        return foundWifiNetworksCount;
    }
    
    Serial.printf("Wi-Fi Manager: WiFi.scanComplete() returned unhandled status: %d\n", scanStatus);
    currentAttemptStatus = WIFI_STATUS_IDLE;
    strncpy(wifiStatusString, "Scan Unknown", sizeof(wifiStatusString)-1);
    wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
    wifiIsScanning = false;
    WiFi.scanDelete();
    return scanStatus;
}


wl_status_t getCurrentWifiStatus() {
    return WiFi.status();
}

String getCurrentSsid() {
    if (WiFi.status() == WL_CONNECTED) {
        currentConnectedSsid = WiFi.SSID();
        return currentConnectedSsid;
    }
    return "";
}

void attemptDirectWifiConnection(const char* ssid) {
    if (!wifiHardwareEnabled) {
        Serial.println("Attempting direct connection but Wi-Fi is off. Enabling first.");
        setWifiHardwareState(true);
        delay(100);
    } else {
        if (strcmp(WiFi.getHostname(), DEVICE_HOSTNAME) != 0) {
            if (WiFi.setHostname(DEVICE_HOSTNAME)) { // Use DEVICE_HOSTNAME from config.h
                Serial.printf("Wi-Fi Manager: Hostname (re)set to '%s' before connecting.\n", DEVICE_HOSTNAME);
            } else {
                Serial.println("Wi-Fi Manager: Failed to (re)set hostname before connecting.");
            }
        }
    }

    Serial.printf("Wi-Fi Manager: Attempting to connect to OPEN network: %s\n", ssid);
    currentConnectedSsid = "";
    WiFi.disconnect(); // Ensure clean state
    delay(100);

    // Explicitly configure for DHCP - this can sometimes help clear old states
    IPAddress emptyIP(0,0,0,0);
    WiFi.config(emptyIP, emptyIP, emptyIP); // Use DHCP
    delay(50); // Short delay after config

    WiFi.begin(ssid);
    connectionStartTime = millis();
    currentAttemptStatus = WIFI_CONNECTING_IN_PROGRESS;
    strncpy(wifiStatusString, "Connecting...", sizeof(wifiStatusString)-1);
    wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
    addOrUpdateKnownNetwork(ssid, "", true);
}

void attemptWpaWifiConnection(const char* ssid, const char* password) {
    if (!wifiHardwareEnabled) {
        Serial.println("Attempting WPA connection but Wi-Fi is off. Enabling first.");
        setWifiHardwareState(true);
        delay(100);
    } else {
        if (strcmp(WiFi.getHostname(), DEVICE_HOSTNAME) != 0) {
             if (WiFi.setHostname(DEVICE_HOSTNAME)) { // Use DEVICE_HOSTNAME from config.h
                Serial.printf("Wi-Fi Manager: Hostname (re)set to '%s' before connecting.\n", DEVICE_HOSTNAME);
            } else {
                Serial.println("Wi-Fi Manager: Failed to (re)set hostname before connecting.");
            }
        }
    }

    Serial.printf("Wi-Fi Manager: Attempting to connect to SECURE network: %s (Pass: ***)\n", ssid);
    currentConnectedSsid = "";
    WiFi.disconnect(); // Ensure clean state
    delay(100);

    // Explicitly configure for DHCP
    IPAddress emptyIP(0,0,0,0);
    WiFi.config(emptyIP, emptyIP, emptyIP); // Use DHCP
    delay(50); // Short delay after config

    WiFi.begin(ssid, password);
    connectionStartTime = millis();
    currentAttemptStatus = WIFI_CONNECTING_IN_PROGRESS;
    strncpy(wifiStatusString, "Connecting...", sizeof(wifiStatusString)-1);
    wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
}

WifiConnectionStatus checkWifiConnectionProgress() {
    if (currentAttemptStatus != WIFI_CONNECTING_IN_PROGRESS) {
        return currentAttemptStatus; 
    }

    wl_status_t status = WiFi.status();
    String connectingSsidAttempt = WiFi.SSID();
    const char* ssidForUpdate = (strlen(currentSsidToConnect) > 0) ? currentSsidToConnect : connectingSsidAttempt.c_str();

    if (status == WL_CONNECTED) {
        Serial.println("Wi-Fi Manager: Connected successfully!");
        currentAttemptStatus = WIFI_CONNECTED_SUCCESS;
        currentConnectedSsid = WiFi.SSID();
        snprintf(wifiStatusString, sizeof(wifiStatusString)-1, "Connected: %s", currentConnectedSsid.c_str());
        if (strlen(ssidForUpdate) > 0) {
            resetSsidFailCount(ssidForUpdate);
        }
    } else if (status == WL_NO_SSID_AVAIL) {
        Serial.println("Wi-Fi Manager: Connection Failed - SSID not available.");
        currentAttemptStatus = WIFI_FAILED_NO_SSID_AVAIL;
        strncpy(wifiStatusString, "SSID Not Found", sizeof(wifiStatusString)-1);
        currentConnectedSsid = ""; 
        if (strlen(ssidForUpdate) > 0) incrementSsidFailCount(ssidForUpdate);
    } else if (status == WL_CONNECT_FAILED) {
        Serial.println("Wi-Fi Manager: Connection Failed - WL_CONNECT_FAILED.");
        if (selectedNetworkIsSecure && strlen(ssidForUpdate) > 0) {
            currentAttemptStatus = WIFI_FAILED_WRONG_PASSWORD;
            strncpy(wifiStatusString, "Auth Error", sizeof(wifiStatusString)-1);
            incrementSsidFailCount(ssidForUpdate);
        } else {
            currentAttemptStatus = WIFI_FAILED_OTHER; 
            strncpy(wifiStatusString, "Connect Fail", sizeof(wifiStatusString)-1);
            if (strlen(ssidForUpdate) > 0) incrementSsidFailCount(ssidForUpdate);
        }
        currentConnectedSsid = ""; 
    } else if (status == WL_IDLE_STATUS || status == WL_DISCONNECTED || status == WL_SCAN_COMPLETED) {
        if (millis() - connectionStartTime > WIFI_CONNECTION_TIMEOUT_MS) {
            Serial.println("Wi-Fi Manager: Connection Timeout.");
            currentAttemptStatus = WIFI_FAILED_TIMEOUT;
            strncpy(wifiStatusString, "Timeout", sizeof(wifiStatusString)-1);
            currentConnectedSsid = ""; 
            if (strlen(ssidForUpdate) > 0) {
                 incrementSsidFailCount(ssidForUpdate);
            }
            WiFi.disconnect(); 
        } else {
            currentAttemptStatus = WIFI_CONNECTING_IN_PROGRESS;
        }
    } else {
        if (millis() - connectionStartTime > WIFI_CONNECTION_TIMEOUT_MS) {
            Serial.printf("Wi-Fi Manager: Timeout with unhandled status %d.\n", status);
            currentAttemptStatus = WIFI_FAILED_TIMEOUT;
            strncpy(wifiStatusString, "Timeout/Error", sizeof(wifiStatusString)-1);
            currentConnectedSsid = ""; 
            if (strlen(ssidForUpdate) > 0) {
                 incrementSsidFailCount(ssidForUpdate);
            }
            WiFi.disconnect();
        } else {
            currentAttemptStatus = WIFI_CONNECTING_IN_PROGRESS;
        }
    }
    wifiStatusString[sizeof(wifiStatusString)-1] = '\0'; 
    return currentAttemptStatus;
}

WifiConnectionStatus getCurrentWifiConnectionStatus() {
    if (!wifiHardwareEnabled) {
        return WIFI_STATUS_IDLE;
    }
    return currentAttemptStatus;
}

const char* getWifiStatusMessage() {
    if (!wifiHardwareEnabled) {
        return "Wi-Fi Off";
    }
    return wifiStatusString;
}