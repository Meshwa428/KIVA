#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "config.h"
#include <WiFi.h>

#define MAX_KNOWN_WIFI_NETWORKS 20
#define MAX_WIFI_FAIL_ATTEMPTS 2

enum WifiConnectionStatus {
    WIFI_STATUS_IDLE,
    WIFI_CONNECTING_IN_PROGRESS,
    WIFI_CONNECTED_SUCCESS,
    WIFI_FAILED_NO_SSID_AVAIL,
    WIFI_FAILED_WRONG_PASSWORD,
    WIFI_FAILED_TIMEOUT,
    WIFI_FAILED_OTHER,
    KIVA_WIFI_SCAN_FAILED,
    KIVA_WIFI_SCAN_RUNNING
};


struct KnownWifiNetwork {
    char ssid[33];
    char password[PASSWORD_MAX_LEN + 1];
    int failCount;
};

// Extern declarations for global variables defined in KivaMain.ino
extern String currentConnectedSsid;
extern KnownWifiNetwork knownNetworksList[MAX_KNOWN_WIFI_NETWORKS];
extern int knownNetworksCount;
// extern bool wifiHardwareEnabled; // Already in config.h -> KivaMain.ino

// Function declarations
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
void updateWifiStatusOnDisconnect(); // <--- NEW

bool loadKnownWifiNetworks();
bool saveKnownWifiNetworks();
KnownWifiNetwork* findKnownNetwork(const char* ssid);
bool addOrUpdateKnownNetwork(const char* ssid, const char* password, bool resetFailCountOnUpdate);
bool incrementSsidFailCount(const char* ssid);
bool resetSsidFailCount(const char* ssid);

void setWifiHardwareState(bool enable, ActiveRfOperationMode modeToSetOnEnable = RF_MODE_NORMAL_STA, int channel = 0); 

#endif // WIFI_MANAGER_H