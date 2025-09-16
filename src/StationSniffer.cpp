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
    if (instance_ && type == WIFI_PKT_DATA) {
        instance_->handlePacket(static_cast<wifi_promiscuous_pkt_t*>(buf));
    }
}

void StationSniffer::handlePacket(wifi_promiscuous_pkt_t *packet) {
    const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)packet;
    const uint8_t *payload = ppkt->payload;

    // Address 1 (DA) is at offset 4, Address 2 (SA) is at offset 10
    const uint8_t *addr1 = payload + 4;
    const uint8_t *addr2 = payload + 10;

    const uint8_t *clientMac = nullptr;

    if (memcmp(addr1, targetAp_.bssid, 6) == 0) {
        clientMac = addr2;
    } else if (memcmp(addr2, targetAp_.bssid, 6) == 0) {
        clientMac = addr1;
    }

    if (clientMac) {
        // Ignore broadcast/multicast
        if (clientMac[0] & 0x01) return;

        StationInfo newStation;
        memcpy(newStation.mac, clientMac, 6);

        bool found = false;
        for (const auto& st : foundStations_) {
            if (st == newStation) {
                found = true;
                break;
            }
        }

        if (!found) {
            foundStations_.push_back(newStation);
            char macStr[18];
            sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", clientMac[0], clientMac[1], clientMac[2], clientMac[3], clientMac[4], clientMac[5]);
            LOG(LogLevel::INFO, "STATION_SNIFFER", "Found new station: %s for AP %s", macStr, targetAp_.ssid);
        }
    }
}

bool StationSniffer::isActive() const { return isActive_; }
const std::vector<StationInfo>& StationSniffer::getFoundStations() const { return foundStations_; }
