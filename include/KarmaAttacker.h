#ifndef KARMA_ATTACKER_H
#define KARMA_ATTACKER_H

#include <vector>
#include <string>
#include "esp_wifi.h"

// Forward declaration to break circular dependency
class App;

// Struct to hold sniffed info
struct SniffedNetworkInfo {
    std::string ssid;
    uint8_t bssid[6];
    int channel;
    bool isSecure;
    // We can add a list of seen clients here later
};

class KarmaAttacker {
public:
    KarmaAttacker();
    void setup(App* app);
    void loop();

    // Attack Control
    void startSniffing();
    void stopSniffing();
    bool launchAttack(const SniffedNetworkInfo& target);
    void stopAttack();

    bool isSniffing() const;
    bool isAttacking() const;
    const std::vector<SniffedNetworkInfo>& getSniffedNetworks() const;
    const std::string& getCurrentTargetSsid() const;

private:
    static void packetHandlerCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    void handlePacket(const wifi_promiscuous_pkt_t *packet);
    void sendDeauthPacket(const uint8_t* targetBssid);

    App* app_;
    bool isSniffing_;
    bool isAttacking_;
    
    std::vector<SniffedNetworkInfo> sniffedNetworks_;
    SniffedNetworkInfo currentTarget_;

    // Channel hopping state
    int channelHopIndex_;
    unsigned long lastChannelHopTime_;

    // Deauth state
    unsigned long lastDeauthTime_;

    static KarmaAttacker* instance_;
};

#endif // KARMA_ATTACKER_H