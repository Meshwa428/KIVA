#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "Config.h"
#include <Arduino.h>
#include <WiFi.h>
#include <vector>

#define PASSWORD_MAX_LEN 63
#define MAX_KNOWN_WIFI_NETWORKS 20
#define WIFI_CONNECTION_TIMEOUT_MS 15000

// Forward declaration
class App;

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

// NEW: Mode for enabling hardware
enum class WifiMode {
    STA,
    AP
};

// Simplified info for UI display
struct WifiNetworkInfo {
    char ssid[33];
    uint8_t bssid[6];
    int32_t channel;
    int8_t rssi;
    bool isSecure;
};

// For storing credentials
struct KnownWifiNetwork {
    char ssid[33];
    char password[PASSWORD_MAX_LEN + 1];
    int failureCount = 0;
};

#include "Service.h"

class WifiManager : public Service {
public:
    WifiManager();
    void setup(App* app) override;
    void loop() override;

    // --- State Control ---
    void setHardwareState(bool enable, WifiMode mode = WifiMode::STA, const char* ap_ssid = nullptr, const char* ap_password = nullptr);
    void startScan();
    void startScanOnChannel(int channel); // <-- NEW
    bool tryConnectKnown(const char* ssid);
    void connectWithPassword(const char* password);
    void connectOpen(const char* ssid);
    void disconnect();

    bool startStationScan(const WifiNetworkInfo& targetNetwork);
    void stopStationScan();

    void onWifiEvent(WiFiEvent_t event, WiFiEventInfo_t info);

    // --- State Getters ---
    WifiState getState() const;
    bool isHardwareEnabled() const;
    const std::vector<WifiNetworkInfo>& getScannedNetworks() const;
    void setSsidToConnect(const char* ssid);
    const char* getSsidToConnect() const;
    String getCurrentSsid() const;
    String getStatusMessage() const;
    uint32_t getScanCompletionCount() const;

    static WifiManager* getInstance();

private:
    void loadKnownNetworks();
    void saveKnownNetworks();
    KnownWifiNetwork* findKnownNetwork(const char* ssid);
    void addOrUpdateKnownNetwork(const char* ssid, const char* password);

    App* app_ = nullptr;
    WifiState state_;
    bool hardwareEnabled_;
    unsigned long connectionStartTime_;
    uint32_t scanCompletionCount_;

    std::vector<WifiNetworkInfo> scannedNetworks_;
    std::vector<KnownWifiNetwork> knownNetworks_;
    bool networksLoaded_ = false;
    char ssidToConnect_[33];
    String statusMessage_;
    static WifiManager* instance_;
};

#endif // WIFI_MANAGER_H