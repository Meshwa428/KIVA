#ifndef HANDSHAKE_CAPTURE_H
#define HANDSHAKE_CAPTURE_H

#include <FS.h>
#include "esp_wifi.h"
#include <vector>
#include <string>
#include <memory>
#include "HardwareManager.h"
#include "WifiManager.h"
#include <map>

class App;

enum class HandshakeCaptureMode {
    EAPOL,
    PMKID
};

enum class TargetedAttackState {
    WAITING_FOR_DEAUTH,
    LISTENING,
    COOLDOWN
};

enum class HandshakeCaptureType {
    SCANNER,
    TARGETED
};

struct HandshakeCaptureConfig {
    HandshakeCaptureMode mode;
    HandshakeCaptureType type;
    WifiNetworkInfo specific_target_info;
};

#include "Service.h"

class HandshakeCapture : public Service {
public:
    HandshakeCapture();
    void setup(App* app) override;
    void loop();

    void prepare(HandshakeCaptureMode mode, HandshakeCaptureType type);
    bool start(const WifiNetworkInfo& targetNetwork); // For targeted
    bool startScanner();
    void stop();

    bool isActive() const;
    const HandshakeCaptureConfig& getConfig() const;
    uint32_t getPacketCount() const;
    int getHandshakeCount() const;
    int getPmkidCount() const;
    TargetedAttackState getTargetedState() const;

private:
    std::vector<std::string> savedHandshakes;
    std::map<std::string, std::string> bssid_to_ssid_map_;

    static void packetHandlerCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    void handlePacket(wifi_promiscuous_pkt_t *packet);

    bool isItEAPOL(const wifi_promiscuous_pkt_t *packet);
    void saveHandshake(const wifi_promiscuous_pkt_t *packet, bool beacon);
    bool writeHeader(File file);

    void parsePMKID(const wifi_promiscuous_pkt_t *packet);

    App* app_;
    std::unique_ptr<HardwareManager::RfLock> rfLock_;
    bool isActive_;
    bool isAttackPending_;
    HandshakeCaptureConfig currentConfig_;

    uint32_t packetCount_;
    int handshakeCount_;
    int pmkidCount_;

    int channelHopIndex_;
    unsigned long lastChannelHopTime_;
    unsigned long channelHopDelayMs_;
    
    TargetedAttackState targetedState_;
    unsigned long lastDeauthTime_;
    unsigned long handshakeCapturedTime_;

    static HandshakeCapture* instance_;
};

#endif // HANDSHAKE_CAPTURE_H