#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "config.h" // For WifiNetwork struct, MAX_WIFI_NETWORKS, and externs
#include <WiFi.h>   // ESP32 WiFi library, provides wl_status_t, WIFI_SCAN_RUNNING etc.

int initiateAsyncWifiScan(); 
int checkAndRetrieveWifiScanResults();
wl_status_t getCurrentWifiStatus();
String getCurrentSsid(); // String type from Arduino.h (via WiFi.h or config.h)
void setupWifi(); 

#endif // WIFI_MANAGER_H