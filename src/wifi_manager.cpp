#include "wifi_manager.h"
#include <Arduino.h>
#include <WiFi.h>
#include <cstring>

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
}

int initiateAsyncWifiScan() {
    Serial.println("Wi-Fi Manager: Initiating async scan...");
    WiFi.scanDelete(); // Clear previous scan results from WiFi class
    delay(50); // Short delay for stability
    // Start an asynchronous scan. Show hidden networks.
    int scanResult = WiFi.scanNetworks(true, true); // async=true, show_hidden=true

    // WiFi.scanNetworks with async=true returns:
    // -1 (WIFI_SCAN_RUNNING) if scan started
    // -2 (WIFI_SCAN_FAILED) if scan failed to start
    // >=0 for synchronous scan (should not happen here)

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
        // This case should ideally not be hit if async is true and working as expected.
        Serial.printf("Wi-Fi Manager: scanNetworks call returned unexpected: %d\n", scanResult);
        // Treat as idle, results will be checked by WiFi.scanComplete()
        currentAttemptStatus = WIFI_STATUS_IDLE;
        strncpy(wifiStatusString, "Scan Done?", sizeof(wifiStatusString)-1); // Or some other status
        wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
    }
    return scanResult; // Return the raw result of WiFi.scanNetworks()
}

int checkAndRetrieveWifiScanResults() {
    int scanStatus = WiFi.scanComplete(); // This is the definitive way to check async scan status

    if (scanStatus == WIFI_SCAN_FAILED) { // -2, from ESP32 WiFi library
        Serial.println("Wi-Fi Manager: WiFi.scanComplete() reported scan FAILED.");
        foundWifiNetworksCount = 0;
        currentAttemptStatus = KIVA_WIFI_SCAN_FAILED; // Our enum
        strncpy(wifiStatusString, "Scan Failed", sizeof(wifiStatusString)-1);
        wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
        WiFi.scanDelete(); // Clean up
        return currentAttemptStatus; // Return our enum value representing the failure
    } else if (scanStatus == WIFI_SCAN_RUNNING) { // -1, from ESP32 WiFi library
        // Serial.println("Wi-Fi Manager: WiFi.scanComplete() reports scan STILL RUNNING.");
        currentAttemptStatus = KIVA_WIFI_SCAN_RUNNING; // Our enum
        // message "Scanning..." already set by initiateAsyncWifiScan
        return currentAttemptStatus; // Return our enum value
    } else if (scanStatus >= 0) { // Scan finished (could be 0 networks, or >0)
        Serial.print("Wi-Fi Manager: Async Scan done, ");
        Serial.print(scanStatus); // Number of networks found by hardware
        Serial.println(" networks found by WiFi.scanComplete().");

        foundWifiNetworksCount = 0; // Reset our count of valid, stored networks

        if (scanStatus > 0) {
            for (int i = 0; i < scanStatus && foundWifiNetworksCount < MAX_WIFI_NETWORKS; ++i) {
                String ssidStr = WiFi.SSID(i);
                ssidStr.trim();

                if (ssidStr.length() == 0 || ssidStr.length() > 32) {
                    continue;
                }

                strncpy(scannedNetworks[foundWifiNetworksCount].ssid, ssidStr.c_str(), 32);
                scannedNetworks[foundWifiNetworksCount].ssid[32] = '\0';
                scannedNetworks[foundWifiNetworksCount].rssi = WiFi.RSSI(i);
                scannedNetworks[foundWifiNetworksCount].isSecure = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
                foundWifiNetworksCount++;
            }

            // Sort the found (and stored) networks by RSSI
            for (int i = 0; i < foundWifiNetworksCount - 1; i++) {
                for (int j = 0; j < foundWifiNetworksCount - i - 1; j++) {
                    if (scannedNetworks[j].rssi < scannedNetworks[j + 1].rssi) {
                        WifiNetwork temp = scannedNetworks[j];
                        scannedNetworks[j] = scannedNetworks[j + 1];
                        scannedNetworks[j + 1] = temp;
                    }
                }
            }
        }

        if (foundWifiNetworksCount == 0) {
             Serial.println("Wi-Fi Manager: No valid networks stored after filtering.");
             strncpy(wifiStatusString, "No Networks", sizeof(wifiStatusString)-1);
        } else {
             snprintf(wifiStatusString, sizeof(wifiStatusString)-1, "%d Networks", foundWifiNetworksCount);
        }
        wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
        currentAttemptStatus = WIFI_STATUS_IDLE; // Scan is done, results processed
        WiFi.scanDelete(); // Clear scan results from WiFi class memory
        return foundWifiNetworksCount; // Return number of networks we stored
    }
    // Should not reach here if scanStatus is one of the defined values.
    // If it does, implies an undefined state from WiFi.scanComplete().
    Serial.printf("Wi-Fi Manager: WiFi.scanComplete() returned unhandled status: %d\n", scanStatus);
    currentAttemptStatus = WIFI_STATUS_IDLE; // Default fallback
    strncpy(wifiStatusString, "Scan Unknown", sizeof(wifiStatusString)-1);
    wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
    return scanStatus; // Return the raw, unhandled status
}


wl_status_t getCurrentWifiStatus() {
    return WiFi.status();
}

String getCurrentSsid() {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.SSID();
    }
    return "";
}

void attemptDirectWifiConnection(const char* ssid) {
    Serial.printf("Wi-Fi Manager: Attempting to connect to OPEN network: %s\n", ssid);
    WiFi.disconnect(); // Disconnect if already connected or attempting
    delay(100);
    WiFi.begin(ssid);
    connectionStartTime = millis();
    currentAttemptStatus = WIFI_CONNECTING_IN_PROGRESS;
    strncpy(wifiStatusString, "Connecting...", sizeof(wifiStatusString)-1);
    wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
}

void attemptWpaWifiConnection(const char* ssid, const char* password) {
    Serial.printf("Wi-Fi Manager: Attempting to connect to SECURE network: %s\n", ssid);
    WiFi.disconnect(); // Disconnect if already connected or attempting
    delay(100);
    WiFi.begin(ssid, password);
    connectionStartTime = millis();
    currentAttemptStatus = WIFI_CONNECTING_IN_PROGRESS;
    strncpy(wifiStatusString, "Connecting...", sizeof(wifiStatusString)-1);
    wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
}

WifiConnectionStatus checkWifiConnectionProgress() {
    if (currentAttemptStatus != WIFI_CONNECTING_IN_PROGRESS) {
        return currentAttemptStatus; // Not currently trying to connect
    }

    wl_status_t status = WiFi.status();

    if (status == WL_CONNECTED) {
        Serial.println("Wi-Fi Manager: Connected successfully!");
        currentAttemptStatus = WIFI_CONNECTED_SUCCESS;
        snprintf(wifiStatusString, sizeof(wifiStatusString)-1, "Connected: %s", WiFi.SSID().c_str());
    } else if (status == WL_NO_SSID_AVAIL) {
        Serial.println("Wi-Fi Manager: Connection Failed - SSID not available.");
        currentAttemptStatus = WIFI_FAILED_NO_SSID_AVAIL;
        strncpy(wifiStatusString, "SSID Not Found", sizeof(wifiStatusString)-1);
    } else if (status == WL_CONNECT_FAILED) {
        Serial.println("Wi-Fi Manager: Connection Failed - WL_CONNECT_FAILED.");
        // This is generic. If `selectedNetworkIsSecure` (global from config.h, set in menu_logic)
        // is true, it's likely a password issue.
        if (selectedNetworkIsSecure) {
            currentAttemptStatus = WIFI_FAILED_WRONG_PASSWORD;
            strncpy(wifiStatusString, "Auth Error", sizeof(wifiStatusString)-1);
        } else {
            currentAttemptStatus = WIFI_FAILED_OTHER; // Generic fail for open network
            strncpy(wifiStatusString, "Connect Fail", sizeof(wifiStatusString)-1);
        }
    } else if (status == WL_IDLE_STATUS || status == WL_DISCONNECTED || status == WL_SCAN_COMPLETED) {
        if (millis() - connectionStartTime > WIFI_CONNECTION_TIMEOUT_MS) {
            Serial.println("Wi-Fi Manager: Connection Timeout.");
            currentAttemptStatus = WIFI_FAILED_TIMEOUT;
            strncpy(wifiStatusString, "Timeout", sizeof(wifiStatusString)-1);
            WiFi.disconnect(); // Stop the attempt explicitly on timeout
        } else {
            // Still in progress, WiFi.status() will eventually change or timeout
            currentAttemptStatus = WIFI_CONNECTING_IN_PROGRESS;
            // "Connecting..." message is already set
        }
    } else {
        // Other unhandled wl_status_t values, treat as still connecting unless timeout
        if (millis() - connectionStartTime > WIFI_CONNECTION_TIMEOUT_MS) {
            Serial.printf("Wi-Fi Manager: Timeout with unhandled status %d.\n", status);
            currentAttemptStatus = WIFI_FAILED_TIMEOUT;
            strncpy(wifiStatusString, "Timeout", sizeof(wifiStatusString)-1);
            WiFi.disconnect();
        } else {
            currentAttemptStatus = WIFI_CONNECTING_IN_PROGRESS;
        }
    }
    wifiStatusString[sizeof(wifiStatusString)-1] = '\0'; // Ensure null termination
    return currentAttemptStatus;
}

WifiConnectionStatus getCurrentWifiConnectionStatus() {
    return currentAttemptStatus;
}

const char* getWifiStatusMessage() {
    return wifiStatusString;
}