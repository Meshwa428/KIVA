#ifndef BEACON_SPAMMER_H
#define BEACON_SPAMMER_H

#include "HardwareManager.h"
#include "SdCardManager.h" // <-- Include our manager instead of SD.h
#include <string>
#include <memory>

class App;

enum class BeaconSsidMode {
    RANDOM,
    FILE_BASED
};

class BeaconSpammer {
public:
    BeaconSpammer();
    ~BeaconSpammer();

    void setup(App* app);
    bool start(std::unique_ptr<HardwareManager::RfLock> rfLock, BeaconSsidMode mode, const std::string& ssidFilePath = "");
    void stop();
    void loop();

    bool isActive() const;
    uint32_t getSsidCounter() const;
    int getCurrentChannel() const;

    static bool isSsidFileValid(const std::string& ssidFilePath);

private:
    struct FakeAP {
        std::string ssid;
        uint8_t bssid[6];
        bool is_wpa2;
    };

    void sendBeaconPacket(const FakeAP& ap);
    FakeAP getNextFakeAP();

    App* app_;
    std::unique_ptr<HardwareManager::RfLock> rfLock_;

    // Attack State
    bool isActive_;
    BeaconSsidMode currentMode_;
    uint8_t currentChannel_;
    
    // --- FIX: Use our new abstract reader ---
    SdCardManager::LineReader ssidReader_;
    uint32_t ssidCounter_;

    // Timing
    unsigned long lastPacketTime_;
    unsigned long lastChannelHopTime_;
    int channelHopIndex_;

    static const int CHANNELS_TO_SPAM[];
};

#endif // BEACON_SPAMMER_H