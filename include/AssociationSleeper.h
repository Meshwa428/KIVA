#ifndef ASSOCIATION_SLEEPER_H
#define ASSOCIATION_SLEEPER_H

#include "HardwareManager.h"
#include "WifiManager.h"
#include "StationSniffer.h"
#include <memory>
#include <vector>

class App;

class AssociationSleeper {
public:
    AssociationSleeper();
    void setup(App* app);
    void loop();

    bool start(const WifiNetworkInfo& ap);
    void stop();

    bool isActive() const;
    uint32_t getPacketCount() const;
    std::string getTargetSsid() const;
    int getClientCount() const;
    bool isSniffing() const;

private:
    void sendSleepPacket(const uint8_t* clientMac);

    App* app_;
    StationSniffer* stationSniffer_;
    std::unique_ptr<HardwareManager::RfLock> rfLock_;
    bool isActive_;
    uint32_t packetCounter_;

    WifiNetworkInfo targetAp_;
    unsigned long lastPacketTime_;
    int currentClientIndex_;
    
    enum class State {
        SNIFFING,
        ATTACKING
    };
    State state_;
};

#endif // ASSOCIATION_SLEEPER_H
