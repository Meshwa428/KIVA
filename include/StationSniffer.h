#ifndef STATION_SNIFFER_H
#define STATION_SNIFFER_H

#include <vector>
#include <string>
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "WifiManager.h"
#include <memory>
#include "HardwareManager.h"
#include "Service.h"

class App;

struct StationInfo {
    uint8_t mac[6];
    // --- NEW: Add the AP's BSSID and channel for context ---
    uint8_t ap_bssid[6];
    int channel;
    
    bool operator==(const StationInfo& other) const {
        return memcmp(mac, other.mac, 6) == 0;
    }
};

class StationSniffer : public Service {
public:
    StationSniffer();
    void setup(App* app) override;
    void loop();

    bool start(const WifiNetworkInfo& targetAp);
    void stop();

    bool isActive() const;
    const std::vector<StationInfo>& getFoundStations() const;

    uint32_t getResourceRequirements() const override;

private:
    static void packetHandlerCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    void handlePacket(wifi_promiscuous_pkt_t *packet);

    App* app_;
    std::unique_ptr<HardwareManager::RfLock> rfLock_;
    bool isActive_;
    
    WifiNetworkInfo targetAp_;
    std::vector<StationInfo> foundStations_;

    static StationSniffer* instance_;
};

#endif // STATION_SNIFFER_H