#ifndef PROBE_FLOODER_H
#define PROBE_FLOODER_H

#include "HardwareManager.h"
#include "SdCardManager.h"
#include <string>
#include <memory>

class App;

enum class ProbeFloodMode {
    RANDOM,
    FILE_BASED
};

#include "Service.h"

class ProbeFlooder : public Service {
public:
    ProbeFlooder();
    ~ProbeFlooder();

    void setup(App* app) override;
    bool start(std::unique_ptr<HardwareManager::RfLock> rfLock, ProbeFloodMode mode, const std::string& ssidFilePath = "");
    void stop();
    void loop();

    bool isActive() const;
    uint32_t getPacketCounter() const;
    int getCurrentChannel() const;

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

    // Timing
    unsigned long lastChannelHopTime_;
    int channelHopIndex_;

    static const int CHANNELS_TO_SPAM[];
};

#endif // PROBE_FLOODER_H