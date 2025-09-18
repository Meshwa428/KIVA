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
    // --- MODIFICATION: New enum ---
    enum class AttackType {
        NORMAL,         // Target a specific AP and all its clients
        BROADCAST,      // Listen on all channels and attack any client seen
        PINPOINT_CLIENT // Attack a single, specific client
    };

    AssociationSleeper();
    void setup(App* app);
    void loop();

    bool start(const WifiNetworkInfo& ap); // For NORMAL
    bool start();                          // For BROADCAST
    bool start(const StationInfo& client); // <-- NEW: For PINPOINT
    void stop();

    bool isActive() const;
    uint32_t getPacketCount() const;
    std::string getTargetSsid() const;
    int getClientCount() const;
    bool isSniffing() const;
    AttackType getAttackType() const; // Renamed from getAttackMode

private:
    void sendSleepPacket(const StationInfo& client);
    
    static void packetHandlerCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    void handlePacket(wifi_promiscuous_pkt_t *packet);

    App* app_;
    StationSniffer* stationSniffer_;
    std::unique_ptr<HardwareManager::RfLock> rfLock_;
    bool isActive_;
    uint32_t packetCounter_;
    AttackType attackType_; // Renamed from attackMode_

    // State for NORMAL mode
    WifiNetworkInfo targetAp_;
    unsigned long lastTargetedPacketTime_;
    int currentClientIndex_;
    enum class State { SNIFFING, ATTACKING };
    State state_;

    // State for BROADCAST mode
    std::vector<StationInfo> newClientsFound_;
    std::map<std::string, unsigned long> recentlyAttacked_;
    int channelHopIndex_;
    unsigned long lastChannelHopTime_;
    
    // --- NEW: State for PINPOINT mode ---
    StationInfo pinpointTarget_;

    static AssociationSleeper* instance_;
};

#endif // ASSOCIATION_SLEEPER_H