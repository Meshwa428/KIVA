#include "WifiManager.h"
#include "SdCardManager.h" // Uses our SD card manager
#include <algorithm> // For std::sort

// Non-member callback function required by the WiFi event system
static void WiFiEventCallback(WiFiEvent_t event, WiFiEventInfo_t info);

// Global pointer to the single WifiManager instance
static WifiManager* wifiManagerInstance = nullptr;

WifiManager::WifiManager() : 
    state_(WifiState::OFF),
    hardwareEnabled_(false),
    connectionStartTime_(0) 
{
    wifiManagerInstance = this;
    ssidToConnect_[0] = '\0';
}

void WifiManager::setup() {
    // Register the event handler
    WiFi.onEvent(WiFiEventCallback);
    loadKnownNetworks();
}

void WifiManager::update() {
    // Handle connection timeout
    if (state_ == WifiState::CONNECTING && (millis() - connectionStartTime_) > WIFI_CONNECTION_TIMEOUT_MS) {
        Serial.println("[WIFI-LOG] Connection timed out.");
        disconnect(); // This will trigger a disconnect event and state change
        state_ = WifiState::CONNECTION_FAILED;
        statusMessage_ = "Timeout";
    }
}

void WifiManager::setHardwareState(bool enable) {
    if (enable) {
        if (!hardwareEnabled_) {
            Serial.println("[WIFI-LOG] Enabling WiFi hardware.");
            WiFi.mode(WIFI_STA);
            hardwareEnabled_ = true;
            state_ = WifiState::IDLE;
            statusMessage_ = "Idle";
        }
    } else {
        if (hardwareEnabled_) {
            Serial.println("[WIFI-LOG] Disabling WiFi hardware.");
            WiFi.disconnect(true, true);
            WiFi.mode(WIFI_OFF);
            hardwareEnabled_ = false;
            state_ = WifiState::OFF;
            statusMessage_ = "WiFi Off";
        }
    }
}

void WifiManager::startScan() {
    if (!hardwareEnabled_) setHardwareState(true);
    if (state_ == WifiState::SCANNING) return;

    Serial.println("[WIFI-LOG] Starting WiFi scan.");
    state_ = WifiState::SCANNING;
    statusMessage_ = "Scanning...";
    WiFi.scanNetworks(true); // Async scan
}

void WifiManager::connectOpen(const char* ssid) {
    if (!hardwareEnabled_) setHardwareState(true);
    setSsidToConnect(ssid);
    state_ = WifiState::CONNECTING;
    statusMessage_ = "Connecting...";
    connectionStartTime_ = millis();
    WiFi.begin(ssidToConnect_);
}

void WifiManager::connectWithPassword(const char* password) {
    if (!hardwareEnabled_) setHardwareState(true);
    addOrUpdateKnownNetwork(ssidToConnect_, password);
    state_ = WifiState::CONNECTING;
    statusMessage_ = "Connecting...";
    connectionStartTime_ = millis();
    WiFi.begin(ssidToConnect_, password);
}

void WifiManager::disconnect() {
    WiFi.disconnect(true);
}

// --- Getters ---
WifiState WifiManager::getState() const { return state_; }
const std::vector<WifiNetworkInfo>& WifiManager::getScannedNetworks() const { return scannedNetworks_; }
const char* WifiManager::getSsidToConnect() const { return ssidToConnect_; }
String WifiManager::getCurrentSsid() const { return (state_ == WifiState::CONNECTED) ? WiFi.SSID() : ""; }
String WifiManager::getStatusMessage() const { return statusMessage_; }
void WifiManager::setSsidToConnect(const char* ssid) {
    strncpy(ssidToConnect_, ssid, sizeof(ssidToConnect_) - 1);
    ssidToConnect_[sizeof(ssidToConnect_) - 1] = '\0';
}


// --- Private Methods ---
void WifiManager::onWifiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.printf("[WIFI-EVENT] Event: %d\n", event);
    switch (event) {
        case ARDUINO_EVENT_WIFI_SCAN_DONE:
        {
            scannedNetworks_.clear();
            int n = WiFi.scanComplete();
            if (n >= 0) {
                 Serial.printf("[WIFI-LOG] Scan complete. %d networks found.\n", n);
                for (int i = 0; i < n; ++i) {
                    WifiNetworkInfo net;
                    strncpy(net.ssid, WiFi.SSID(i).c_str(), sizeof(net.ssid)-1);
                    net.ssid[sizeof(net.ssid)-1] = '\0';
                    net.rssi = WiFi.RSSI(i);
                    net.isSecure = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
                    net.isConnected = (state_ == WifiState::CONNECTED && WiFi.SSID() == WiFi.SSID(i));
                    scannedNetworks_.push_back(net);
                }
                // Sort by RSSI
                std::sort(scannedNetworks_.begin(), scannedNetworks_.end(), [](const WifiNetworkInfo& a, const WifiNetworkInfo& b) {
                    return a.rssi > b.rssi;
                });
                state_ = WifiState::IDLE;
                statusMessage_ = String(n) + " networks";
            } else {
                Serial.println("[WIFI-LOG] Scan failed.");
                state_ = WifiState::SCAN_FAILED;
                statusMessage_ = "Scan Failed";
            }
            break;
        }

        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Serial.println("[WIFI-LOG] Station Connected to AP.");
            statusMessage_ = "Connected";
            break;

        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.printf("[WIFI-LOG] Got IP: %s\n", WiFi.localIP().toString().c_str());
            state_ = WifiState::CONNECTED;
            statusMessage_ = "Connected: " + WiFi.SSID();
            addOrUpdateKnownNetwork(ssidToConnect_, findKnownNetwork(ssidToConnect_)->password);
            break;

        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Serial.printf("[WIFI-LOG] Disconnected. Reason: %d\n", info.wifi_sta_disconnected.reason);
            state_ = WifiState::IDLE;
            statusMessage_ = "Disconnected";
            if (connectionStartTime_ != 0) { // If we were trying to connect
                state_ = WifiState::CONNECTION_FAILED;
                statusMessage_ = "Auth Error";
            }
            connectionStartTime_ = 0;
            break;
            
        default:
            break;
    }
}

void WifiManager::loadKnownNetworks() {
    knownNetworks_.clear();
    String content = SdCardManager::readFile(KNOWN_WIFI_FILE);
    int currentIndex = 0;
    while(currentIndex < content.length()) {
        int ssidEnd = content.indexOf(';', currentIndex);
        if (ssidEnd == -1) break;
        int passEnd = content.indexOf('\n', ssidEnd + 1);
        if (passEnd == -1) passEnd = content.length();
        
        KnownWifiNetwork net;
        strncpy(net.ssid, content.substring(currentIndex, ssidEnd).c_str(), sizeof(net.ssid)-1);
        net.ssid[sizeof(net.ssid)-1] = '\0';
        strncpy(net.password, content.substring(ssidEnd + 1, passEnd).c_str(), sizeof(net.password)-1);
        net.password[sizeof(net.password)-1] = '\0';
        knownNetworks_.push_back(net);
        
        currentIndex = passEnd + 1;
    }
    Serial.printf("[WIFI-LOG] Loaded %d known networks.\n", knownNetworks_.size());
}

void WifiManager::saveKnownNetworks() {
    String content = "";
    for (const auto& net : knownNetworks_) {
        content += net.ssid;
        content += ";";
        content += net.password;
        content += "\n";
    }
    SdCardManager::writeFile(KNOWN_WIFI_FILE, content.c_str());
    Serial.printf("[WIFI-LOG] Saved %d known networks.\n", knownNetworks_.size());
}

KnownWifiNetwork* WifiManager::findKnownNetwork(const char* ssid) {
    for (auto& net : knownNetworks_) {
        if (strcmp(net.ssid, ssid) == 0) {
            return &net;
        }
    }
    return nullptr;
}

void WifiManager::addOrUpdateKnownNetwork(const char* ssid, const char* password) {
    KnownWifiNetwork* existing = findKnownNetwork(ssid);
    if (existing) {
        if (strcmp(existing->password, password) != 0) {
            strncpy(existing->password, password, sizeof(existing->password)-1);
            existing->password[sizeof(existing->password)-1] = '\0';
            saveKnownNetworks();
        }
    } else {
        if (knownNetworks_.size() < MAX_KNOWN_WIFI_NETWORKS) {
            KnownWifiNetwork newNet;
            strncpy(newNet.ssid, ssid, sizeof(newNet.ssid)-1);
            newNet.ssid[sizeof(newNet.ssid)-1] = '\0';
            strncpy(newNet.password, password, sizeof(newNet.password)-1);
            newNet.password[sizeof(newNet.password)-1] = '\0';
            knownNetworks_.push_back(newNet);
            saveKnownNetworks();
        }
    }
}


// --- Static C-style callback function ---
void WiFiEventCallback(WiFiEvent_t event, WiFiEventInfo_t info) {
    if (wifiManagerInstance) {
        wifiManagerInstance->onWifiEvent(event, info);
    }
}