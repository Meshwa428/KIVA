#ifndef DEAUTHER_H
#define DEAUTHER_H

#include "HardwareManager.h"
#include "WifiManager.h"
#include "StationSniffer.h" // <-- NEW: For StationInfo struct
#include <string>
#include <vector>
#include <memory>

class App;

// --- MODIFICATION: A single, more descriptive enum for attack types ---
enum class DeauthAttackType {
    NORMAL,             // Deauth a specific AP (broadcast frames to its clients)
    EVIL_TWIN,          // Deauth a specific AP (using its BSSID to create a rogue AP)
    BROADCAST_NORMAL,   // Cycle through all APs with normal deauth
    BROADCAST_EVIL_TWIN,// Cycle through all APs with rogue AP deauth
    PINPOINT_CLIENT     // Deauth a single, specific client
};

// Configuration for an attack
struct DeauthConfig {
    DeauthAttackType type;
    WifiNetworkInfo specific_ap_info;
    StationInfo specific_client_info; // <-- NEW
};

#include "Service.h"

class Deauther : public Service {
public:
    Deauther();
    void setup(App* app) override;
    void loop();

    // --- Attack Control ---
    void prepareAttack(DeauthAttackType type);
    bool start(const WifiNetworkInfo& targetNetwork); // For AP-based attacks
    bool start(const StationInfo& targetClient);      // <-- NEW: For pinpoint attacks
    bool startBroadcast();                            // For cycling attacks
    void stop();

    // --- State Getters ---
    bool isActive() const;
    bool isAttackPending() const;
    const DeauthConfig& getPendingConfig() const;
    const std::string& getCurrentTargetSsid() const;

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

    static const uint32_t ALL_APS_HOP_INTERVAL_MS = 15000;
    static const uint32_t PACKET_INTERVAL_MS = 200; // For NORMAL and PINPOINT modes

    // Timing for packets
    unsigned long lastPacketSendTime_;
};

#endif // DEAUTHER_H