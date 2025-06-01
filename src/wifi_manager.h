#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "config.h"
#include <WiFi.h>   // ESP32 WiFi library, provides wl_status_t, WIFI_SCAN_RUNNING etc.

// Enum for detailed connection status
enum WifiConnectionStatus {
    WIFI_STATUS_IDLE,
    WIFI_CONNECTING_IN_PROGRESS,
    WIFI_CONNECTED_SUCCESS,
    WIFI_FAILED_NO_SSID_AVAIL,
    WIFI_FAILED_WRONG_PASSWORD,
    WIFI_FAILED_TIMEOUT,
    WIFI_FAILED_OTHER,
    KIVA_WIFI_SCAN_FAILED,  // <--- RENAMED: From checkAndRetrieveWifiScanResults if scan hardware fails
    KIVA_WIFI_SCAN_RUNNING  // <--- RENAMED: From checkAndRetrieveWifiScanResults if scan is ongoing
};


int initiateAsyncWifiScan();
int checkAndRetrieveWifiScanResults(); // Returns raw WiFi.scanComplete() or our KIVA_WIFI_ enums for scan status
wl_status_t getCurrentWifiStatus(); // Raw ESP32 status
String getCurrentSsid();
void setupWifi();

// New functions for connection attempts
void attemptDirectWifiConnection(const char* ssid);
void attemptWpaWifiConnection(const char* ssid, const char* password);
WifiConnectionStatus checkWifiConnectionProgress();
WifiConnectionStatus getCurrentWifiConnectionStatus(); // To get the last known status for WIFI_CONNECTION_INFO
const char* getWifiStatusMessage(); // Get a human-readable message for the current status

#endif // WIFI_MANAGER_H