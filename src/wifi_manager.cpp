#include "wifi_manager.h"
#include <Arduino.h>
#include <WiFi.h>
#include <cstring>
#include <algorithm> // For std::sort

// Definition for the new global variable
String currentConnectedSsid = "";

// Static variable to store the current connection attempt status and message
static WifiConnectionStatus currentAttemptStatus = WIFI_STATUS_IDLE;
static unsigned long connectionStartTime = 0;
const unsigned long WIFI_CONNECTION_TIMEOUT_MS = 15000; // 15 seconds timeout
static char wifiStatusString[50] = "Idle";


void setupWifi() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(); // Disconnect from any previous session
    delay(100);
    Serial.println("Wi-Fi Manager: STA Mode Initialized.");
    currentAttemptStatus = WIFI_STATUS_IDLE;
    strncpy(wifiStatusString, "Idle", sizeof(wifiStatusString)-1);
    wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
    currentConnectedSsid = ""; // Ensure it's empty on setup
}

int initiateAsyncWifiScan() {
    Serial.println("Wi-Fi Manager: Initiating async scan...");
    WiFi.scanDelete(); // Clear previous scan results from WiFi class
    delay(50); // Short delay for stability
    // Start an asynchronous scan. Show hidden networks.
    int scanResult = WiFi.scanNetworks(true, true); // async=true, show_hidden=true

    if (scanResult == WIFI_SCAN_FAILED) { // -2, defined by ESP32 WiFi library
        Serial.println("Wi-Fi Manager: Scan initiation failed.");
        currentAttemptStatus = KIVA_WIFI_SCAN_FAILED; // Use our enum
        strncpy(wifiStatusString, "Scan Failed", sizeof(wifiStatusString)-1);
        wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
    } else if (scanResult == WIFI_SCAN_RUNNING) { // -1, defined by ESP32 WiFi library
        Serial.println("Wi-Fi Manager: Scan started successfully, running in background.");
        currentAttemptStatus = KIVA_WIFI_SCAN_RUNNING; // Use our enum
        strncpy(wifiStatusString, "Scanning...", sizeof(wifiStatusString)-1);
        wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
    } else {
        Serial.printf("Wi-Fi Manager: scanNetworks call returned unexpected: %d\n", scanResult);
        currentAttemptStatus = WIFI_STATUS_IDLE;
        strncpy(wifiStatusString, "Scan Done?", sizeof(wifiStatusString)-1); 
        wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
    }
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
        WiFi.scanDelete(); 
        return currentAttemptStatus; 
    } else if (scanStatus == WIFI_SCAN_RUNNING) { 
        currentAttemptStatus = KIVA_WIFI_SCAN_RUNNING; 
        return currentAttemptStatus; 
    } else if (scanStatus >= 0) { 
        Serial.print("Wi-Fi Manager: Async Scan done, ");
        Serial.print(scanStatus); 
        Serial.println(" networks found by WiFi.scanComplete().");

        foundWifiNetworksCount = 0; 
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

        int connectedIdx = -1;
        if (currentConnectedSsid.length() > 0) {
            for (int i = 0; i < tempCount; ++i) {
                if (currentConnectedSsid.equals(tempScannedNetworks[i].ssid)) {
                    connectedIdx = i;
                    break;
                }
            }
        }

        foundWifiNetworksCount = 0;
        if (connectedIdx != -1) {
            scannedNetworks[foundWifiNetworksCount++] = tempScannedNetworks[connectedIdx];
        }

        for (int i = 0; i < tempCount; ++i) {
            if (i == connectedIdx) continue; 
            if (foundWifiNetworksCount < MAX_WIFI_NETWORKS) {
                scannedNetworks[foundWifiNetworksCount++] = tempScannedNetworks[i];
            } else {
                break; 
            }
        }

        if (foundWifiNetworksCount == 0) {
             Serial.println("Wi-Fi Manager: No valid networks stored after filtering.");
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
    return scanStatus; 
}


wl_status_t getCurrentWifiStatus() {
    return WiFi.status();
}

String getCurrentSsid() {
    if (WiFi.status() == WL_CONNECTED) {
        currentConnectedSsid = WiFi.SSID(); // Keep our global updated
        return currentConnectedSsid;
    }
    // If not connected, currentConnectedSsid will retain its last connected value or be empty.
    // It's cleared during new connection attempts or on failure.
    return ""; // Return empty if not actively connected at this moment
}

void attemptDirectWifiConnection(const char* ssid) {
    Serial.printf("Wi-Fi Manager: Attempting to connect to OPEN network: %s\n", ssid);
    currentConnectedSsid = ""; // Clear previous connected SSID before attempt
    WiFi.disconnect(); 
    delay(100);
    WiFi.begin(ssid);
    connectionStartTime = millis();
    currentAttemptStatus = WIFI_CONNECTING_IN_PROGRESS;
    strncpy(wifiStatusString, "Connecting...", sizeof(wifiStatusString)-1);
    wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
}

void attemptWpaWifiConnection(const char* ssid, const char* password) {
    Serial.printf("Wi-Fi Manager: Attempting to connect to SECURE network: %s\n", ssid);
    currentConnectedSsid = ""; // Clear previous connected SSID before attempt
    WiFi.disconnect(); 
    delay(100);
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

    if (status == WL_CONNECTED) {
        Serial.println("Wi-Fi Manager: Connected successfully!");
        currentAttemptStatus = WIFI_CONNECTED_SUCCESS;
        currentConnectedSsid = WiFi.SSID(); // Update global here
        snprintf(wifiStatusString, sizeof(wifiStatusString)-1, "Connected: %s", currentConnectedSsid.c_str());
    } else if (status == WL_NO_SSID_AVAIL) {
        Serial.println("Wi-Fi Manager: Connection Failed - SSID not available.");
        currentAttemptStatus = WIFI_FAILED_NO_SSID_AVAIL;
        strncpy(wifiStatusString, "SSID Not Found", sizeof(wifiStatusString)-1);
        currentConnectedSsid = ""; 
    } else if (status == WL_CONNECT_FAILED) {
        Serial.println("Wi-Fi Manager: Connection Failed - WL_CONNECT_FAILED.");
        if (selectedNetworkIsSecure) {
            currentAttemptStatus = WIFI_FAILED_WRONG_PASSWORD;
            strncpy(wifiStatusString, "Auth Error", sizeof(wifiStatusString)-1);
        } else {
            currentAttemptStatus = WIFI_FAILED_OTHER; 
            strncpy(wifiStatusString, "Connect Fail", sizeof(wifiStatusString)-1);
        }
        currentConnectedSsid = ""; 
    } else if (status == WL_IDLE_STATUS || status == WL_DISCONNECTED || status == WL_SCAN_COMPLETED) {
        if (millis() - connectionStartTime > WIFI_CONNECTION_TIMEOUT_MS) {
            Serial.println("Wi-Fi Manager: Connection Timeout.");
            currentAttemptStatus = WIFI_FAILED_TIMEOUT;
            strncpy(wifiStatusString, "Timeout", sizeof(wifiStatusString)-1);
            currentConnectedSsid = ""; 
            WiFi.disconnect(); 
        } else {
            currentAttemptStatus = WIFI_CONNECTING_IN_PROGRESS;
        }
    } else {
        if (millis() - connectionStartTime > WIFI_CONNECTION_TIMEOUT_MS) {
            Serial.printf("Wi-Fi Manager: Timeout with unhandled status %d.\n", status);
            currentAttemptStatus = WIFI_FAILED_TIMEOUT;
            strncpy(wifiStatusString, "Timeout", sizeof(wifiStatusString)-1);
            currentConnectedSsid = ""; 
            WiFi.disconnect();
        } else {
            currentAttemptStatus = WIFI_CONNECTING_IN_PROGRESS;
        }
    }
    wifiStatusString[sizeof(wifiStatusString)-1] = '\0'; 
    return currentAttemptStatus;
}

WifiConnectionStatus getCurrentWifiConnectionStatus() {
    return currentAttemptStatus;
}

const char* getWifiStatusMessage() {
    return wifiStatusString;
}