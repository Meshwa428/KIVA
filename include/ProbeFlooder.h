#ifndef PROBE_FLOODER_H
#define PROBE_FLOODER_H

#include "HardwareManager.h"
#include "SdCardManager.h"
#include <string>
#include <memory>
#include <vector>
#include "WifiManager.h" // Added for WifiNetworkInfo

class App;

enum class ProbeFloodMode {
    RANDOM,
    FILE_BASED,
    PINPOINT_AP,
    AUTO_BROADCAST_PINPOINT,
    AUTO_SNIFFED_PINPOINT
};

enum class AutoPinpointState {
    SCANNING,
    FLOODING
};

#include "Service.h"

class ProbeFlooder : public Service {
public:
    ProbeFlooder();
    ~ProbeFlooder();

    void setup(App* app) override;

    // Overloaded start methods
    bool start(std::unique_ptr<HardwareManager::RfLock> rfLock, ProbeFloodMode mode); // For RANDOM, FILE_BASED, and AUTO modes
    bool start(std::unique_ptr<HardwareManager::RfLock> rfLock, const WifiNetworkInfo& targetNetwork); // For PINPOINT_AP

    void stop();
    void loop();

    bool isActive() const;
    uint32_t getPacketCounter() const;
    int getCurrentChannel() const;
    const std::string getCurrentTargetSsid() const;
    ProbeFloodMode getMode() const;
    AutoPinpointState getAutoPinpointState() const;

    uint32_t getResourceRequirements() const override;

private:
    std::string getNextSsid();
    void sendProbePacket(const std::string& ssid);

    App* app_;
    std::unique_ptr<HardwareManager::RfLock> rfLock_;

    // Attack State
    bool isActive_;
    ProbeFloodMode currentMode_;
    uint8_t currentChannel_;
    
    SdCardManager::LineReader fileSsidReader_;
    uint32_t packetCounter_;

    // State for Pinpoint mode
    WifiNetworkInfo targetAp_;

    // State for Auto Broadcast Pinpoint modes
    AutoPinpointState autoPinpointState_;
    std::vector<WifiNetworkInfo> currentTargetsOnChannel_;
    int currentTargetIndex_;
    unsigned long lastTargetHopTime_;
    uint32_t lastScanCount_;
    
    // State for Auto Sniffed Pinpoint mode
    std::vector<std::string> sniffedSsids_;
    int sniffedSsidIndex_;

    // Timing
    unsigned long lastChannelHopTime_;
    int channelHopIndex_;
};

#endif // PROBE_FLOODER_H