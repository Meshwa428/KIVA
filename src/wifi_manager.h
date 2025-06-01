#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "config.h"
#include <WiFi.h>   // ESP32 WiFi library, provides wl_status_t, WIFI_SCAN_RUNNING etc.

#define MAX_KNOWN_WIFI_NETWORKS 20 // Max number of Wi-Fi networks to store
#define MAX_WIFI_FAIL_ATTEMPTS 2   // Max connection failures before asking for password again

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


// Structure to hold known Wi-Fi network details
struct KnownWifiNetwork {
    char ssid[33];
    char password[PASSWORD_MAX_LEN + 1]; // From config.h
    int failCount;
};

// Extern declarations for known networks list
extern String currentConnectedSsid;
extern KnownWifiNetwork knownNetworksList[MAX_KNOWN_WIFI_NETWORKS];
extern int knownNetworksCount;

// Function declarations (existing and new)
void setupWifi();
int initiateAsyncWifiScan();
int checkAndRetrieveWifiScanResults();
wl_status_t getCurrentWifiStatus();
String getCurrentSsid();

void attemptDirectWifiConnection(const char* ssid);
void attemptWpaWifiConnection(const char* ssid, const char* password);
WifiConnectionStatus checkWifiConnectionProgress();
WifiConnectionStatus getCurrentWifiConnectionStatus();
const char* getWifiStatusMessage();

// New functions for managing known networks
bool loadKnownWifiNetworks();
bool saveKnownWifiNetworks();
KnownWifiNetwork* findKnownNetwork(const char* ssid);
bool addOrUpdateKnownNetwork(const char* ssid, const char* password, bool resetFailCountOnUpdate);
bool incrementSsidFailCount(const char* ssid); // Renamed for clarity
bool resetSsidFailCount(const char* ssid);     // Renamed for clarity

// New function for explicit Wi-Fi power control
void setWifiHardwareState(bool enable); 



#endif // WIFI_MANAGER_H