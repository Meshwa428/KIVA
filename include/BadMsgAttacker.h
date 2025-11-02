#ifndef BAD_MSG_ATTACKER_H
#define BAD_MSG_ATTACKER_H

#include "Service.h"
#include "HardwareManager.h"
#include "WifiManager.h"
#include "StationSniffer.h"
#include <memory>
#include <vector>

class App;

class BadMsgAttacker : public Service {
public:
    enum class AttackType { TARGET_AP, PINPOINT_CLIENT, FROM_FILE };
    
    // State machine for the attack cycle
    enum class AttackPhase {
        SNIFFING,
        ATTACKING,
        ATTACK_ONLY
    };

    struct AttackConfig {
        AttackType type;
        WifiNetworkInfo targetAp;
        StationInfo targetClient;
        std::string filePath;
    };

    BadMsgAttacker();
    void setup(App* app) override;
    void loop() override;

    void prepareAttack(AttackType type);
    bool start(const WifiNetworkInfo& targetAp);
    bool start(const StationInfo& targetClient);
    bool start(const std::string& filePath, const WifiNetworkInfo& targetAp);
    void stop();

    bool isActive() const;
    const AttackConfig& getConfig() const;
    uint32_t getPacketCount() const;
    int getClientCount() const;
    bool isSniffing() const; // This will now represent the SNIFFING phase

    uint32_t getResourceRequirements() const override;

private:
    void sendBadMsgPacket(const StationInfo& client, uint8_t securityType);
    
    // Helper functions for the state machine
    void switchToSniffPhase();
    void switchToAttackPhase();
    void switchToAttackOnlyPhase();
    void attackNextClient();

    // Packet handler is now internal to this class for TARGET_AP mode
    static void packetHandlerCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    void handlePacket(wifi_promiscuous_pkt_t *packet);

    App* app_;
    StationSniffer* stationSniffer_; // Still needed for the initial pinpoint scan
    std::unique_ptr<HardwareManager::RfLock> rfLock_;

    bool isActive_;
    bool isAttackPending_;
    AttackConfig currentConfig_;
    uint32_t packetCounter_;
    unsigned long lastPacketTime_;

    AttackPhase currentPhase_;
    unsigned long lastPhaseSwitchTime_;

    std::vector<StationInfo> knownClients_;
    int currentClientIndex_;
    int cyclesWithoutDiscovery_;

    static BadMsgAttacker* instance_; // Static instance for the callback
};

#endif // BAD_MSG_ATTACKER_H