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
    networksLoaded_(false), // Initialize flag
    connectionStartTime_(0),
    scanCompletionCount_(0)
{
    wifiManagerInstance = this;
    ssidToConnect_[0] = '\0';
}

void WifiManager::setup(App* app) {
    app_ = app;
    WiFi.onEvent(WiFiEventCallback);
    WiFi.mode(WIFI_OFF);
    statusMessage_ = "WiFi Off";
}

void WifiManager::update() {
    if (state_ == WifiState::CONNECTING && (millis() - connectionStartTime_) > WIFI_CONNECTION_TIMEOUT_MS) {
        Serial.println("[WIFI-LOG] Connection timed out.");
        state_ = WifiState::CONNECTION_FAILED;
        statusMessage_ = "Timeout";
        disconnect(); // Call disconnect to clean up
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
    } else {
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
            
            knownNetworks_.clear();
            networksLoaded_ = false;
            Serial.println("[WIFI-LOG] Cleared known networks from RAM.");
        }
    }
}


void WifiManager::startScan() {
    if (!hardwareEnabled_ || WiFi.getMode() != WIFI_STA) {
        return; 
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
        Serial.printf("[WIFI-LOG] Found known network '%s' with %d failures. Trying to auto-connect.\n", ssid, known->failureCount);
        setSsidToConnect(ssid);
        state_ = WifiState::CONNECTING;
        statusMessage_ = "Connecting...";
        connectionStartTime_ = millis();
        WiFi.begin(ssid, known->password);
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
    // This is now a full state reset function.
    if (state_ == WifiState::IDLE || state_ == WifiState::OFF) return;
    
    Serial.println("[WIFI-LOG] Disconnecting and resetting state.");
    state_ = WifiState::IDLE;
    statusMessage_ = "Disconnected";
    connectionStartTime_ = 0;
    ssidToConnect_[0] = '\0';
    WiFi.disconnect(true);
}

// --- Getters ---
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
    Serial.printf("[WIFI-EVENT] Event: %d\n", event);
    switch (event) {
        case ARDUINO_EVENT_WIFI_SCAN_DONE:
        {
            scanCompletionCount_++;
            int n = WiFi.scanComplete();
            if (n < 0) {
                if (state_ != WifiState::CONNECTED) {
                    state_ = WifiState::SCAN_FAILED;
                    statusMessage_ = "Scan Failed";
                }
                return;
            }
            
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
                statusMessage_ = String(scannedNetworks_.size()) + " networks";
            }
            break;
        }

        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        { 
            state_ = WifiState::CONNECTED;
            statusMessage_ = "Connected: " + WiFi.SSID();
            
            KnownWifiNetwork* known = findKnownNetwork(WiFi.SSID().c_str());
            if(known && known->failureCount > 0) {
                known->failureCount = 0;
                saveKnownNetworks();
            }
            
            if (app_) {
                app_->executePostWifiAction();
            }
            break;
        } 

        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        {
            wifi_event_sta_disconnected_t disconnected_info = info.wifi_sta_disconnected;
            
            char event_ssid[33] = {0};
            uint8_t len_to_copy = disconnected_info.ssid_len < sizeof(event_ssid) - 1 ? disconnected_info.ssid_len : sizeof(event_ssid) - 1;
            memcpy(event_ssid, disconnected_info.ssid, len_to_copy);

            Serial.printf("[WIFI-EVENT] STA_DISCONNECTED from SSID: '%s', reason: %d\n", event_ssid, disconnected_info.reason);

            // --- CRITICAL FIX: Only treat as a failure if we were trying to connect to THIS specific SSID ---
            if (state_ == WifiState::CONNECTING && strcmp(ssidToConnect_, event_ssid) == 0) {
                Serial.printf("[WIFI-LOG] Disconnect matches current attempt for '%s'. Marking as failed.\n", ssidToConnect_);
                
                state_ = WifiState::CONNECTION_FAILED;
                
                switch(disconnected_info.reason) {
                    case WIFI_REASON_AUTH_FAIL:
                    case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:
                    case WIFI_REASON_AUTH_EXPIRE:
                        statusMessage_ = "Auth Error";
                        break;
                    case WIFI_REASON_NO_AP_FOUND:
                        statusMessage_ = "AP Not Found";
                        break;
                    default:
                        statusMessage_ = "Connect Failed";
                        break;
                }

                KnownWifiNetwork* known = findKnownNetwork(ssidToConnect_);
                if (known) {
                    known->failureCount++;
                    Serial.printf("[WIFI-LOG] Failure count for '%s' is now %d.\n", ssidToConnect_, known->failureCount);
                    saveKnownNetworks();
                }
            } else if (state_ != WifiState::CONNECTING) {
                // This was a disconnect from a previously active session, not a failed connection attempt.
                state_ = WifiState::IDLE;
                statusMessage_ = "Disconnected";
                Serial.println("[WIFI-LOG] Previous session disconnected. State is now IDLE.");
            } else {
                // This is a latent disconnect event from a previous attempt that arrived late.
                // We are already trying to connect to a new network, so we ignore this old event.
                Serial.printf("[WIFI-LOG] Ignoring latent disconnect event for '%s' while connecting to '%s'.\n", event_ssid, ssidToConnect_);
            }
            
            connectionStartTime_ = 0;
            break;
        }
            
        default:
            break;
    }
}

// --- MODIFIED load/save functions ---
void WifiManager::loadKnownNetworks() {
    if (networksLoaded_) return;
    knownNetworks_.clear();
    String content = SdCardManager::readFile(KNOWN_WIFI_FILE);
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
    Serial.printf("[WIFI-LOG] LAZY LOAD: Loaded %d known networks from SD into RAM.\n", knownNetworks_.size());
}

void WifiManager::saveKnownNetworks() {
    if (!networksLoaded_) return;
    String content = "";
    for (const auto& net : knownNetworks_) {
        content += net.ssid;
        content += ";";
        content += net.password;
        content += ";";
        content += String(net.failureCount);
        content += "\n";
    }
    SdCardManager::writeFile(KNOWN_WIFI_FILE, content.c_str());
    Serial.printf("[WIFI-LOG] Saved %d known networks.\n", knownNetworks_.size());
}

KnownWifiNetwork* WifiManager::findKnownNetwork(const char* ssid) {
    if (!networksLoaded_) {
        loadKnownNetworks();
    }
    for (auto& net : knownNetworks_) {
        if (strcmp(net.ssid, ssid) == 0) {
            return &net;
        }
    }
    return nullptr;
}

void WifiManager::addOrUpdateKnownNetwork(const char* ssid, const char* password) {
    if (!networksLoaded_) {
        loadKnownNetworks();
    }
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


// --- Static C-style callback function ---
void WiFiEventCallback(WiFiEvent_t event, WiFiEventInfo_t info) {
    if (wifiManagerInstance) {
        wifiManagerInstance->onWifiEvent(event, info);
    }
}