#ifndef ASSOCIATION_SLEEPER_H
#define ASSOCIATION_SLEEPER_H

#include "HardwareManager.h"
#include "WifiManager.h"
#include "StationSniffer.h"
#include <memory>
#include <vector>
#include <map>

class App;

class AssociationSleeper {
public:
    enum class AttackMode {
        TARGETED,
        BROADCAST
    };

    AssociationSleeper();
    void setup(App* app);
    void loop();

    bool start(const WifiNetworkInfo& ap);
    bool start(); // For broadcast
    void stop();

    bool isActive() const;
    uint32_t getPacketCount() const;
    std::string getTargetSsid() const;
    int getClientCount() const;
    bool isSniffing() const;
    AttackMode getAttackMode() const; // NEW getter for UI

private:
    // --- MODIFIED: Needs AP info now ---
    void sendSleepPacket(const StationInfo& client);
    
    // --- NEW: Packet handler for broadcast mode ---
    static void packetHandlerCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    void handlePacket(wifi_promiscuous_pkt_t *packet);

    App* app_;
    StationSniffer* stationSniffer_;
    std::unique_ptr<HardwareManager::RfLock> rfLock_;
    bool isActive_;
    uint32_t packetCounter_;
    AttackMode attackMode_;

    // State for TARGETED mode
    WifiNetworkInfo targetAp_;
    unsigned long lastTargetedPacketTime_;
    int currentClientIndex_;
    enum class State { SNIFFING, ATTACKING };
    State state_;

    // --- NEW: State for BROADCAST mode ---
    std::vector<StationInfo> newClientsFound_;
    std::map<std::string, unsigned long> recentlyAttacked_;
    int channelHopIndex_;
    unsigned long lastChannelHopTime_;

    static AssociationSleeper* instance_;
};

#endif // ASSOCIATION_SLEEPER_H