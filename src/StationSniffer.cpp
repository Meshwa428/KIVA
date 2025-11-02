#include "StationSniffer.h"
#include "App.h"
#include "Logger.h"
#include <Arduino.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include <esp_system.h>
#include <esp_interface.h>

StationSniffer* StationSniffer::instance_ = nullptr;

StationSniffer::StationSniffer() : app_(nullptr), isActive_(false) {
    instance_ = this;
}

void StationSniffer::setup(App* app) {
    app_ = app;
}

bool StationSniffer::start(const WifiNetworkInfo& targetAp) {
    if (isActive_) stop();
    LOG(LogLevel::INFO, "STATION_SNIFFER", "Starting station scan for %s", targetAp.ssid);

    rfLock_ = app_->getHardwareManager().requestRfControl(RfClient::WIFI_PROMISCUOUS);
    if (!rfLock_ || !rfLock_->isValid()) {
        LOG(LogLevel::ERROR, "STATION_SNIFFER", "Failed to acquire RF lock.");
        return false;
    }

    targetAp_ = targetAp;
    foundStations_.clear();
    
    esp_wifi_set_promiscuous_rx_cb(&packetHandlerCallback);
    esp_wifi_set_channel(targetAp_.channel, WIFI_SECOND_CHAN_NONE);

    isActive_ = true;
    return true;
}

void StationSniffer::stop() {
    if (!isActive_) return;
    LOG(LogLevel::INFO, "STATION_SNIFFER", "Stopping station scan.");
    isActive_ = false;
    rfLock_.reset();
}

void StationSniffer::loop() {
    // Nothing to do in loop, it's all in the callback
}

void StationSniffer::packetHandlerCallback(void* buf, wifi_promiscuous_pkt_type_t type) {
    if (instance_ && (type == WIFI_PKT_MGMT || type == WIFI_PKT_DATA)) {
        instance_->handlePacket(static_cast<wifi_promiscuous_pkt_t*>(buf));
    }
}

void StationSniffer::handlePacket(wifi_promiscuous_pkt_t *packet) {
    const uint8_t *payload = packet->payload;
    const uint16_t frame_control = payload[0] | (payload[1] << 8);

    const bool to_ds = (frame_control >> 8) & 0x01;
    const bool from_ds = (frame_control >> 9) & 0x01;

    const uint8_t *bssid = nullptr;
    const uint8_t *client_mac = nullptr;

    if (!to_ds && from_ds) {
        bssid = payload + 10; 
        client_mac = payload + 4;
    } else if (to_ds && !from_ds) {
        bssid = payload + 4;
        client_mac = payload + 10;
    } else {
        return;
    }

    if (memcmp(bssid, targetAp_.bssid, 6) == 0) {
        if (client_mac[0] & 0x01) return;

        StationInfo newStation;
        memcpy(newStation.mac, client_mac, 6);

        bool found = false;
        for (const auto& st : foundStations_) {
            if (st == newStation) {
                found = true;
                break;
            }
        }

        if (!found) {
            // --- NEW: Populate all fields ---
            memcpy(newStation.ap_bssid, bssid, 6);
            newStation.channel = targetAp_.channel;
            foundStations_.push_back(newStation);
            
            char macStr[18];
            sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", client_mac[0], client_mac[1], client_mac[2], client_mac[3], client_mac[4], client_mac[5]);
            LOG(LogLevel::INFO, "STATION_SNIFFER", "Found new station: %s for AP %s", macStr, targetAp_.ssid);
        }
    }
}

bool StationSniffer::isActive() const { return isActive_; }
const std::vector<StationInfo>& StationSniffer::getFoundStations() const { return foundStations_; }
uint32_t StationSniffer::getResourceRequirements() const {
    return isActive_ ? (uint32_t)ResourceRequirement::WIFI : (uint32_t)ResourceRequirement::NONE;
}