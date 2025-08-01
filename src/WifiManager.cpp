#include "WifiManager.h"
#include "SdCardManager.h"
#include "App.h"
#include <algorithm>
#include <ESPmDNS.h>

// Non-member callback function required by the WiFi event system
static void WiFiEventCallback(WiFiEvent_t event, WiFiEventInfo_t info);

// Global pointer to the single WifiManager instance
static WifiManager* wifiManagerInstance = nullptr;

WifiManager::WifiManager() : 
    app_(nullptr),
    state_(WifiState::OFF),
    hardwareEnabled_(false),
    networksLoaded_(false),
    connectionStartTime_(0),
    scanCompletionCount_(0)
{
    wifiManagerInstance = this;
    ssidToConnect_[0] = '\0';
}

void WifiManager::setup(App* app) {
    app_ = app;
    WiFi.onEvent(WiFiEventCallback);
    WiFi.setAutoReconnect(false);
    WiFi.mode(WIFI_OFF);
    statusMessage_ = "WiFi Off";
}

void WifiManager::update() {
    if (state_ == WifiState::CONNECTING && (millis() - connectionStartTime_) > WIFI_CONNECTION_TIMEOUT_MS) {
        Serial.println("[WIFI-LOG] Connection definitively timed out.");
        state_ = WifiState::CONNECTION_FAILED; // Set the final state here.

        // Increment the failure counter here, as this is the true failure point.
        KnownWifiNetwork* known = findKnownNetwork(ssidToConnect_);
        if (known) {
            known->failureCount++;
            Serial.printf("[WIFI-LOG] Connection to '%s' failed on timeout. Count is now %d.\n", known->ssid, known->failureCount);
            saveKnownNetworks();
        }

        // Set the final status message.
        statusMessage_ = "Timeout";

        // We must explicitly stop the connection attempt now.
        WiFi.disconnect();
    }
}

void WifiManager::setHardwareState(bool enable, WifiMode mode, const char* ap_ssid, const char* ap_password) {
    if (enable) {
        WifiMode currentMode = (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) ? WifiMode::AP : WifiMode::STA;
        if(hardwareEnabled_ && state_ != WifiState::OFF && mode == currentMode) {
             return;
        }

        Serial.println("[WIFI-LOG] Re-configuring WiFi hardware...");
        setHardwareState(false);
        delay(200);

        if(mode == WifiMode::STA) {
            Serial.println("[WIFI-LOG] Enabling WiFi hardware (STA mode).");
            if (WiFi.mode(WIFI_STA)) {
                hardwareEnabled_ = true;
                state_ = WifiState::IDLE;
                statusMessage_ = "Idle";
            } else {
                 hardwareEnabled_ = false; state_ = WifiState::OFF; statusMessage_ = "Enable Fail";
            }
        } else if (mode == WifiMode::AP) {
            Serial.println("[WIFI-LOG] Enabling WiFi hardware (AP mode).");
            if (WiFi.mode(WIFI_AP)) {
                IPAddress apIP(192, 168, 4, 1);
                WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
                if (WiFi.softAP(ap_ssid, ap_password)) {
                    hardwareEnabled_ = true;
                    state_ = WifiState::IDLE;
                    statusMessage_ = "AP Active";
                    if (MDNS.begin(Firmware::OTA_HOSTNAME)) {
                        MDNS.addService("http", "tcp", 80);
                    }
                } else {
                     hardwareEnabled_ = false; state_ = WifiState::OFF; statusMessage_ = "AP Fail";
                }
            } else {
                 hardwareEnabled_ = false; state_ = WifiState::OFF; statusMessage_ = "Enable Fail";
            }
        }
    } else {
        if (hardwareEnabled_) {
            LOG(LogLevel::WARN, "WIFI_MANAGER", "Disabling radio hardware completely.");
            MDNS.end();
            WiFi.disconnect(true, true);
            WiFi.softAPdisconnect(true);
            WiFi.mode(WIFI_OFF);
            hardwareEnabled_ = false;
            state_ = WifiState::OFF;
            statusMessage_ = "WiFi Off";
            scannedNetworks_.clear();
            knownNetworks_.clear();
            networksLoaded_ = false;
        }
    }
}

void WifiManager::startScan() {
    if (!hardwareEnabled_ || WiFi.getMode() != WIFI_STA) {
        setHardwareState(true, WifiMode::STA);
        delay(100);
    }
    if (state_ == WifiState::SCANNING) return;

    if (state_ != WifiState::CONNECTED) {
        state_ = WifiState::SCANNING;
        statusMessage_ = "Scanning...";
    }
    WiFi.scanNetworks(true);
}


bool WifiManager::tryConnectKnown(const char* ssid) {
    if (!networksLoaded_) {
        loadKnownNetworks();
    }

    KnownWifiNetwork* known = findKnownNetwork(ssid);
    const int MAX_FAILURES = 3;

    if (known && known->failureCount < MAX_FAILURES) {
        Serial.printf("[WIFI-LOG] Found known network '%s' (Failures: %d). Trying to auto-connect.\n", ssid, known->failureCount);
        connectWithPassword(known->password);
        return true;
    }
    
    if (known) {
        Serial.printf("[WIFI-LOG] Known network '%s' has too many failures (%d). Forcing password entry.\n", ssid, known->failureCount);
    } else {
        Serial.printf("[WIFI-LOG] Network '%s' is not known. Forcing password entry.\n", ssid);
    }
    return false;
}

void WifiManager::connectOpen(const char* ssid) {
    if (!hardwareEnabled_ || WiFi.getMode() != WIFI_STA) setHardwareState(true, WifiMode::STA);
    WiFi.disconnect();
    setSsidToConnect(ssid);
    state_ = WifiState::CONNECTING;
    statusMessage_ = "Connecting...";
    connectionStartTime_ = millis();
    WiFi.begin(ssidToConnect_);
}

void WifiManager::connectWithPassword(const char* password) {
    if (!hardwareEnabled_ || WiFi.getMode() != WIFI_STA) setHardwareState(true, WifiMode::STA);
    WiFi.disconnect();
    addOrUpdateKnownNetwork(ssidToConnect_, password);
    state_ = WifiState::CONNECTING;
    statusMessage_ = "Connecting...";
    connectionStartTime_ = millis();
    WiFi.begin(ssidToConnect_, password);
}

void WifiManager::disconnect() {
    if (state_ != WifiState::CONNECTED) return;
    WiFi.disconnect(true);
    state_ = WifiState::IDLE;
    statusMessage_ = "Disconnected";
    connectionStartTime_ = 0;
}

WifiState WifiManager::getState() const { return state_; }
bool WifiManager::isHardwareEnabled() const { return hardwareEnabled_; }
const std::vector<WifiNetworkInfo>& WifiManager::getScannedNetworks() const { return scannedNetworks_; }
const char* WifiManager::getSsidToConnect() const { return ssidToConnect_; }
String WifiManager::getCurrentSsid() const {
    if (state_ == WifiState::CONNECTED) {
        return WiFi.SSID();
    }
    return "";
}
String WifiManager::getStatusMessage() const { return statusMessage_; }

void WifiManager::setSsidToConnect(const char* ssid) {
    strncpy(ssidToConnect_, ssid, sizeof(ssidToConnect_) - 1);
    ssidToConnect_[sizeof(ssidToConnect_) - 1] = '\0';
}

uint32_t WifiManager::getScanCompletionCount() const {
    return scanCompletionCount_;
}

void WifiManager::onWifiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_SCAN_DONE: {
            scanCompletionCount_++;
            int n = WiFi.scanComplete();
            if (n < 0) {
                if (state_ == WifiState::SCANNING) {
                    state_ = WifiState::SCAN_FAILED;
                    statusMessage_ = "Scan Failed";
                }
            } else {
                scannedNetworks_.clear(); 
                for (int i = 0; i < n; ++i) {
                    WifiNetworkInfo net;
                    strncpy(net.ssid, WiFi.SSID(i).c_str(), sizeof(net.ssid)-1);
                    net.ssid[sizeof(net.ssid)-1] = '\0';
                    memcpy(net.bssid, WiFi.BSSID(i), 6);
                    net.channel = WiFi.channel(i);
                    net.rssi = WiFi.RSSI(i);
                    net.isSecure = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
                    scannedNetworks_.push_back(net);
                }
                std::sort(scannedNetworks_.begin(), scannedNetworks_.end(), [](const WifiNetworkInfo& a, const WifiNetworkInfo& b) { return a.rssi > b.rssi; });
                if (state_ == WifiState::SCANNING) {
                    state_ = WifiState::IDLE;
                    statusMessage_ = String(scannedNetworks_.size()) + " networks";
                }
            }
            break;
        }

        case ARDUINO_EVENT_WIFI_STA_GOT_IP: { 
            state_ = WifiState::CONNECTED;
            statusMessage_ = "Connected";
            
            KnownWifiNetwork* known = findKnownNetwork(WiFi.SSID().c_str());
            if(known && known->failureCount > 0) {
                Serial.printf("[WIFI-LOG] Connection to '%s' successful. Resetting failure count.\n", known->ssid);
                known->failureCount = 0;
                saveKnownNetworks();
            }
            break;
        } 

        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED: {
            wifi_event_sta_disconnected_t disconnected_info = info.wifi_sta_disconnected;
            if (state_ == WifiState::CONNECTING) {
                Serial.printf("[WIFI-LOG] Intermediate disconnect during connection. Reason: %d. Waiting for timeout or success.\n", disconnected_info.reason);
                switch(disconnected_info.reason) {
                    case WIFI_REASON_AUTH_FAIL:
                    case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
                    case WIFI_REASON_AUTH_EXPIRE:
                        statusMessage_ = "Wrong Password..."; break;
                    default:
                        statusMessage_ = "Connecting..."; break;
                }
            } else if (state_ == WifiState::CONNECTED) {
                state_ = WifiState::IDLE;
                statusMessage_ = "Disconnected";
            }
            break;
        }
        default: break;
    }
}

void WifiManager::loadKnownNetworks() {
    if (networksLoaded_) return;
    knownNetworks_.clear();
    String content = SdCardManager::readFile(SD_ROOT::WIFI_KNOWN_NETWORKS);
    int currentIndex = 0;
    while(currentIndex < content.length()) {
        int ssidEnd = content.indexOf(';', currentIndex);
        if (ssidEnd == -1) break;
        int passEnd = content.indexOf(';', ssidEnd + 1);
        if (passEnd == -1) break;
        int failEnd = content.indexOf('\n', passEnd + 1);
        if (failEnd == -1) failEnd = content.length();
        KnownWifiNetwork net;
        strncpy(net.ssid, content.substring(currentIndex, ssidEnd).c_str(), sizeof(net.ssid)-1);
        net.ssid[sizeof(net.ssid)-1] = '\0';
        strncpy(net.password, content.substring(ssidEnd + 1, passEnd).c_str(), sizeof(net.password)-1);
        net.password[sizeof(net.password)-1] = '\0';
        net.failureCount = content.substring(passEnd + 1, failEnd).toInt();
        knownNetworks_.push_back(net);
        currentIndex = failEnd + 1;
    }
    networksLoaded_ = true;
}

void WifiManager::saveKnownNetworks() {
    if (!networksLoaded_) return;
    String content = "";
    for (const auto& net : knownNetworks_) {
        content += net.ssid; content += ";";
        content += net.password; content += ";";
        content += String(net.failureCount); content += "\n";
    }
    SdCardManager::writeFile(SD_ROOT::WIFI_KNOWN_NETWORKS, content.c_str());
}

KnownWifiNetwork* WifiManager::findKnownNetwork(const char* ssid) {
    if (!networksLoaded_) { loadKnownNetworks(); }
    for (auto& net : knownNetworks_) {
        if (strcmp(net.ssid, ssid) == 0) { return &net; }
    }
    return nullptr;
}

void WifiManager::addOrUpdateKnownNetwork(const char* ssid, const char* password) {
    if (!networksLoaded_) { loadKnownNetworks(); }
    KnownWifiNetwork* existing = findKnownNetwork(ssid);
    if (existing) {
        if (strcmp(existing->password, password) != 0 || existing->failureCount > 0) {
            strncpy(existing->password, password, sizeof(existing->password)-1);
            existing->password[sizeof(existing->password)-1] = '\0';
            existing->failureCount = 0;
            saveKnownNetworks();
        }
    } else {
        if (knownNetworks_.size() < MAX_KNOWN_WIFI_NETWORKS) {
            KnownWifiNetwork newNet;
            strncpy(newNet.ssid, ssid, sizeof(newNet.ssid)-1);
            newNet.ssid[sizeof(newNet.ssid)-1] = '\0';
            strncpy(newNet.password, password, sizeof(newNet.password)-1);
            newNet.password[sizeof(newNet.password)-1] = '\0';
            newNet.failureCount = 0;
            knownNetworks_.push_back(newNet);
            saveKnownNetworks();
        }
    }
}

void WiFiEventCallback(WiFiEvent_t event, WiFiEventInfo_t info) {
    if (wifiManagerInstance) {
        wifiManagerInstance->onWifiEvent(event, info);
    }
}