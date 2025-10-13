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
    enum class AttackType { TARGET_AP, PINPOINT_CLIENT };

    struct AttackConfig {
        AttackType type;
        WifiNetworkInfo targetAp;
        StationInfo targetClient;
    };

    BadMsgAttacker();
    void setup(App* app) override;
    void loop() override;

    void prepareAttack(AttackType type);
    bool start(const WifiNetworkInfo& targetAp);
    bool start(const StationInfo& targetClient);
    void stop();

    bool isActive() const;
    const AttackConfig& getConfig() const;
    uint32_t getPacketCount() const;
    int getClientCount() const;
    bool isSniffing() const;

private:
    void sendBadMsgPacket(const StationInfo& client, uint8_t securityType);

    App* app_;
    StationSniffer* stationSniffer_;
    std::unique_ptr<HardwareManager::RfLock> rfLock_;

    bool isActive_;
    bool isAttackPending_;
    AttackConfig currentConfig_;
    uint32_t packetCounter_;
    unsigned long lastPacketTime_;

    // For TARGET_AP mode
    bool isSniffing_;
    int currentClientIndex_;
};

#endif // BAD_MSG_ATTACKER_H