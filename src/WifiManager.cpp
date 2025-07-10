#include "WifiManager.h"
#include "SdCardManager.h"
#include "App.h" // <-- Include App.h
#include <algorithm>
#include <ESPmDNS.h>

// Non-member callback function required by the WiFi event system
static void WiFiEventCallback(WiFiEvent_t event, WiFiEventInfo_t info);

// Global pointer to the single WifiManager instance
static WifiManager* wifiManagerInstance = nullptr;

WifiManager::WifiManager() : 
    app_(nullptr), // <-- Initialize new member
    state_(WifiState::OFF),
    hardwareEnabled_(false),
    connectionStartTime_(0),
    scanCompletionCount_(0)
{
    wifiManagerInstance = this;
    ssidToConnect_[0] = '\0';
}

void WifiManager::setup(App* app) { // <-- Modified signature
    app_ = app; // <-- Store the app instance
    WiFi.onEvent(WiFiEventCallback);
    // Initial state is OFF, so don't enable it here.
    WiFi.mode(WIFI_OFF); // Redundant if never turned on, but safe.
    loadKnownNetworks();
    statusMessage_ = "WiFi Off"; // Set initial status message
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

void WifiManager::setHardwareState(bool enable, WifiMode mode, const char* ap_ssid, const char* ap_password) {
    if (enable) {
        WifiMode currentMode = (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) ? WifiMode::AP : WifiMode::STA;
        if(hardwareEnabled_ && state_ != WifiState::OFF && mode == currentMode) {
             Serial.printf("[WIFI-LOG] Already in correct WiFi mode (%d). No action.\n", (int)mode);
             return;
        }

        Serial.println("[WIFI-LOG] Re-configuring WiFi hardware...");
        setHardwareState(false); // Clean shutdown first
        delay(200);

        if(mode == WifiMode::STA) {
            Serial.println("[WIFI-LOG] Enabling WiFi hardware (STA mode).");
            if (WiFi.mode(WIFI_STA)) {
                hardwareEnabled_ = true;
                state_ = WifiState::IDLE;
                statusMessage_ = "Idle";
                Serial.println("[WIFI-LOG] WiFi STA mode enabled.");
            } else {
                 Serial.println("[WIFI-LOG] ERROR: WiFi.mode(WIFI_STA) failed!");
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
                    Serial.printf("[WIFI-LOG] AP Mode Enabled. SSID: %s, IP: %s\n", ap_ssid, WiFi.softAPIP().toString().c_str());
                    if (MDNS.begin(Firmware::OTA_HOSTNAME)) {
                        MDNS.addService("http", "tcp", 80);
                        Serial.println("[WIFI-LOG] MDNS responder started.");
                    }
                } else {
                     Serial.println("[WIFI-LOG] ERROR: WiFi.softAP() failed!");
                     hardwareEnabled_ = false; state_ = WifiState::OFF; statusMessage_ = "AP Fail";
                }
            } else {
                 Serial.println("[WIFI-LOG] ERROR: WiFi.mode(WIFI_AP) failed!");
                 hardwareEnabled_ = false; state_ = WifiState::OFF; statusMessage_ = "Enable Fail";
            }
        }
    } else { // disable
        if (hardwareEnabled_) {
            Serial.println("[WIFI-LOG] Disabling WiFi hardware.");
            MDNS.end();
            WiFi.disconnect(true, true);
            WiFi.softAPdisconnect(true);
            WiFi.mode(WIFI_OFF);
            hardwareEnabled_ = false;
            state_ = WifiState::OFF;
            statusMessage_ = "WiFi Off";
            scannedNetworks_.clear();
            Serial.println("[WIFI-LOG] WiFi hardware disabled.");
        }
    }
}


void WifiManager::startScan() {
    if (!hardwareEnabled_ || WiFi.getMode() != WIFI_STA) {
        Serial.println("[WIFI-LOG] Scan requested but hardware is OFF or not in STA mode. Ignoring.");
        return; 
    }
    if (state_ == WifiState::SCANNING) return;

    Serial.println("[WIFI-LOG] Starting WiFi scan.");
    
    if (state_ != WifiState::CONNECTED) {
        state_ = WifiState::SCANNING;
        statusMessage_ = "Scanning...";
    }
    WiFi.scanNetworks(true);
}

void WifiManager::connectOpen(const char* ssid) {
    if (!hardwareEnabled_ || WiFi.getMode() != WIFI_STA) setHardwareState(true, WifiMode::STA);
    setSsidToConnect(ssid);
    state_ = WifiState::CONNECTING;
    statusMessage_ = "Connecting...";
    connectionStartTime_ = millis();
    WiFi.begin(ssidToConnect_);
}

void WifiManager::connectWithPassword(const char* password) {
    if (!hardwareEnabled_ || WiFi.getMode() != WIFI_STA) setHardwareState(true, WifiMode::STA);
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
bool WifiManager::isHardwareEnabled() const { return hardwareEnabled_; }
const std::vector<WifiNetworkInfo>& WifiManager::getScannedNetworks() const { return scannedNetworks_; }
const char* WifiManager::getSsidToConnect() const { return ssidToConnect_; }
String WifiManager::getCurrentSsid() const {
    if (state_ == WifiState::CONNECTED) {
        String currentSsid = WiFi.SSID();
        return currentSsid;
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
    Serial.printf("[WIFI-EVENT] Event: %d\n", event);
    switch (event) {
        case ARDUINO_EVENT_WIFI_SCAN_DONE:
        {
            scanCompletionCount_++;
            int n = WiFi.scanComplete();
            if (n < 0) {
                Serial.println("[WIFI-LOG] Scan failed.");
                scannedNetworks_.clear(); 
                if (state_ != WifiState::CONNECTED) {
                    state_ = WifiState::SCAN_FAILED;
                    statusMessage_ = "Scan Failed";
                }
                return;
            }
            
            Serial.printf("[WIFI-LOG] Scan complete. %d networks found.\n", n);
            scannedNetworks_.clear(); 

            for (int i = 0; i < n; ++i) {
                WifiNetworkInfo net;
                strncpy(net.ssid, WiFi.SSID(i).c_str(), sizeof(net.ssid)-1);
                net.ssid[sizeof(net.ssid)-1] = '\0';
                net.rssi = WiFi.RSSI(i);
                net.isSecure = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
                scannedNetworks_.push_back(net);
            }
            
            std::sort(scannedNetworks_.begin(), scannedNetworks_.end(), [](const WifiNetworkInfo& a, const WifiNetworkInfo& b) {
                return a.rssi > b.rssi;
            });

            if (state_ != WifiState::CONNECTED) {
                state_ = WifiState::IDLE;
            }

            statusMessage_ = String(scannedNetworks_.size()) + " networks";
            break;
        }

        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        { 
            Serial.printf("[WIFI-LOG] Got IP: %s\n", WiFi.localIP().toString().c_str());
            state_ = WifiState::CONNECTED;
            statusMessage_ = "Connected: " + WiFi.SSID();
            
            const char* usedPassword = "";
            KnownWifiNetwork* known = findKnownNetwork(ssidToConnect_);
            if(known) {
                usedPassword = known->password;
            }
            addOrUpdateKnownNetwork(ssidToConnect_, usedPassword);

            // --- NEW: Execute pending action ---
            if (app_) {
                app_->executePostWifiAction();
            }

            delay(100);
            break;
        } 

        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        { 
            Serial.printf("[WIFI-LOG] Disconnected. Reason: %d\n", info.wifi_sta_disconnected.reason);
            
            if (state_ == WifiState::CONNECTING) { 
                state_ = WifiState::CONNECTION_FAILED;
                statusMessage_ = "Auth Error";
            } else {
                state_ = WifiState::IDLE;
                statusMessage_ = "Disconnected";
            }
            connectionStartTime_ = 0;
            break;
        }
            
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