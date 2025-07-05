// in src/wifi_attack_tools.h

#ifndef WIFI_ATTACK_TOOLS_H
#define WIFI_ATTACK_TOOLS_H

#include "config.h"
#include "rf_tools_common.h"
#include "esp_wifi.h" // For esp_wifi_80211_tx

// Enum for the different SSID spam modes
enum class BeaconSsidMode {
    UNINITIALIZED,
    RANDOM,
    FROM_FILE
};

// Extern flag to be defined in wifi_attack_tools.cpp
extern bool isBeaconSpamActive;

// --- Beacon Spam Functions ---
void start_beacon_spam(bool isRickRollMode, int channel = 1);
void stop_beacon_spam();
void run_beacon_spam_cycle();
BeaconSsidMode get_beacon_ssid_mode();

#endif // WIFI_ATTACK_TOOLS_H