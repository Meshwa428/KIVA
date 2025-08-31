#ifndef PROBE_SNIFFER_H
#define PROBE_SNIFFER_H

#include <FS.h>
#include "esp_wifi.h"
#include <vector>
#include <string>
#include <memory>
#include "HardwareManager.h"

// Forward declaration to avoid circular dependencies
class App; 

class ProbeSniffer {
public:
    ProbeSniffer();
    void setup(App* app);
    void loop();

    bool start(); 
    void stop();

    bool isActive() const;
    uint32_t getPacketCount() const;
    const std::vector<std::string>& getUniqueSsids() const; // For UI display

private:
    void openPcapFile();
    void closePcapFile();

    // The static callback required by the ESP-IDF WiFi library
    static void packetHandlerCallback(void* buf, wifi_promiscuous_pkt_type_t type);
    // The instance method that does the actual work
    void handlePacket(wifi_promiscuous_pkt_t *packet); 

    App* app_;
    std::unique_ptr<HardwareManager::RfLock> rfLock_;
    bool isActive_;
    uint32_t packetCount_;
    
    // --- Data Storage ---
    File pcapFile_;
    char currentPcapFilename_[64];
    std::vector<std::string> uniqueSsids_; // To show the user what's been found

    // --- Channel Hopping ---
    int channelHopIndex_;
    unsigned long lastChannelHopTime_;
    
    // Static instance pointer to bridge the C-style callback to our C++ class
    static ProbeSniffer* instance_;
};

#endif // PROBE_SNIFFER_H