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
int checkAndRetrieveWifiScanResults();
wl_status_t getCurrentWifiStatus();
String getCurrentSsid(); // Gets SSID if connected, otherwise empty
void setupWifi();

void attemptDirectWifiConnection(const char* ssid);
void attemptWpaWifiConnection(const char* ssid, const char* password);
WifiConnectionStatus checkWifiConnectionProgress();
WifiConnectionStatus getCurrentWifiConnectionStatus();
const char* getWifiStatusMessage();

// New global variable to store the connected SSID for UI purposes
extern String currentConnectedSsid; // Definition will be in wifi_manager.cpp

#endif // WIFI_MANAGER_H