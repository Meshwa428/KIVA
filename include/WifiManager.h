#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "Config.h"
#include <Arduino.h>
#include <WiFi.h>
#include <vector>

#define PASSWORD_MAX_LEN 63
#define MAX_KNOWN_WIFI_NETWORKS 20
#define WIFI_CONNECTION_TIMEOUT_MS 15000

// State of the WifiManager itself
enum class WifiState {
    OFF,
    IDLE,
    SCANNING,
    SCAN_FAILED,
    CONNECTING,
    CONNECTED,
    CONNECTION_FAILED
};

// Simplified info for UI display
struct WifiNetworkInfo {
    char ssid[33];
    int8_t rssi;
    bool isSecure;
};

// For storing credentials
struct KnownWifiNetwork {
    char ssid[33];
    char password[PASSWORD_MAX_LEN + 1];
};

class WifiManager {
public:
    WifiManager();
    void setup();
    void update();

    // --- State Control ---
    void setHardwareState(bool enable);
    void startScan();
    void connectWithPassword(const char* password);
    void connectOpen(const char* ssid); // Takes ssid for open networks
    void disconnect();

    void onWifiEvent(WiFiEvent_t event, WiFiEventInfo_t info);

    // --- State Getters ---
    WifiState getState() const;
    const std::vector<WifiNetworkInfo>& getScannedNetworks() const;
    void setSsidToConnect(const char* ssid);
    const char* getSsidToConnect() const;
    String getCurrentSsid() const;
    String getStatusMessage() const;
    uint32_t getScanCompletionCount() const; // <-- ADD THIS

private:
    void loadKnownNetworks();
    void saveKnownNetworks();
    KnownWifiNetwork* findKnownNetwork(const char* ssid);
    void addOrUpdateKnownNetwork(const char* ssid, const char* password);

    WifiState state_;
    bool hardwareEnabled_;
    unsigned long connectionStartTime_;
    uint32_t scanCompletionCount_; // <-- ADD THIS

    std::vector<WifiNetworkInfo> scannedNetworks_;
    std::vector<KnownWifiNetwork> knownNetworks_;
    char ssidToConnect_[33];
    String statusMessage_;
};

#endif // WIFI_MANAGER_H