#ifndef DEAUTHER_H
#define DEAUTHER_H

#include "HardwareManager.h"
#include "WifiManager.h"
#include <string>
#include <vector>
#include <memory>

class App;

// Defines the attack methodology
enum class DeauthMode {
    ROGUE_AP,
    BROADCAST
};

// Defines the target selection strategy
enum class DeauthTarget {
    SPECIFIC_AP,
    ALL_APS
};

// Configuration for an attack
struct DeauthConfig {
    DeauthMode mode;
    DeauthTarget target;
    WifiNetworkInfo specific_target_info;
};

class Deauther {
public:
    Deauther();
    void setup(App* app);
    void loop();

    // --- Attack Control ---
    void prepareAttack(DeauthMode mode, DeauthTarget target);
    bool start(const WifiNetworkInfo& targetNetwork); // For specific AP attacks
    bool startAllAPs();                               // For cycling attacks
    void stop();

    // --- State Getters ---
    bool isActive() const;
    bool isAttackPending() const;
    const DeauthConfig& getPendingConfig() const;
    const std::string& getCurrentTargetSsid() const;

    /**
     * @brief Sends a burst of deauthentication packets to a target on a specific channel.
     * This is a static utility function that can be called from anywhere in the app.
     * It handles channel switching internally.
     * @param targetBssid The 6-byte BSSID of the target access point.
     * @param channel The channel the target AP is on.
     * @param clientMac The 6-byte MAC address of a specific client to deauth. If nullptr, a broadcast deauth is sent.
     */
    static void sendPacket(const uint8_t* targetBssid, int channel, const uint8_t* clientMac = nullptr);

private:
    void hopToNextTarget();
    void executeAttackForCurrentTarget();

    App* app_;
    std::unique_ptr<HardwareManager::RfLock> rfLock_;

    // Attack State
    bool isActive_;
    bool isAttackPending_;
    DeauthConfig currentConfig_;
    std::string currentTargetSsid_;
    
    // State for cycling through all APs
    std::vector<WifiNetworkInfo> allTargets_;
    int currentTargetIndex_;
    unsigned long lastHopTime_;

    static const uint32_t ALL_APS_HOP_INTERVAL_MS = 15000; // Attack each AP for 15 seconds
    static const uint32_t BROADCAST_PACKET_INTERVAL_MS = 200;

    // Timing for broadcast packets
    unsigned long lastPacketSendTime_;
};

#endif // DEAUTHER_H