#ifndef PROBE_FLOODER_H
#define PROBE_FLOODER_H

#include "HardwareManager.h"
#include "SdCardManager.h"
#include <string>
#include <memory>
#include "WifiManager.h" // Added for WifiNetworkInfo

class App;

enum class ProbeFloodMode {
    RANDOM,
    FILE_BASED,
    PINPOINT_AP // <-- NEW: Pinpoint mode
};

#include "Service.h"

class ProbeFlooder : public Service {
public:
    ProbeFlooder();
    ~ProbeFlooder();

    void setup(App* app) override;

    // --- Overloaded start methods ---
    bool start(std::unique_ptr<HardwareManager::RfLock> rfLock, ProbeFloodMode mode, const std::string& ssidFilePath = "");
    bool start(std::unique_ptr<HardwareManager::RfLock> rfLock, const WifiNetworkInfo& targetNetwork); // <-- NEW

    void stop();
    void loop();

    bool isActive() const;
    uint32_t getPacketCounter() const;
    int getCurrentChannel() const;
    const WifiNetworkInfo& getTargetAp() const; // <-- NEW
    ProbeFloodMode getMode() const; // <-- NEW

private:
    std::string getNextSsid();
    void sendProbePacket(const std::string& ssid);

    App* app_;
    std::unique_ptr<HardwareManager::RfLock> rfLock_;

    // Attack State
    bool isActive_;
    ProbeFloodMode currentMode_;
    uint8_t currentChannel_;
    
    SdCardManager::LineReader ssidReader_;
    uint32_t packetCounter_;

    // --- NEW: State for Pinpoint mode ---
    WifiNetworkInfo targetAp_;

    // Timing
    unsigned long lastChannelHopTime_;
    int channelHopIndex_;

    static const int CHANNELS_TO_SPAM[];
};

#endif // PROBE_FLOODER_H