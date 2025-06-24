#ifndef RF_TOOLS_COMMON_H
#define RF_TOOLS_COMMON_H

#include <Arduino.h>
#include <LinkedList.h> // For dynamic lists, leveraging PSRAM

// Example Kiva-specific data structures (will be expanded in later phases)
// For now, these are just placeholders to illustrate.

struct KivaAccessPoint {
    String essid;
    uint8_t bssid[6];
    int channel;
    int8_t rssi;
    uint8_t security; // 0: Open, 1: WEP, 2: WPA, 3: WPA2, 4: WPA3 etc.
    bool selected;
    // Add other fields as needed from WiFiScan's AccessPoint struct
    // LinkedList<uint16_t>* stations; // Example if we track stations per AP

    KivaAccessPoint() : channel(0), rssi(0), security(0), selected(false) {
        memset(bssid, 0, sizeof(bssid));
        // stations = new LinkedList<uint16_t>();
    }

    // Basic MAC address to string for convenience
    String bssidToString() const {
        char macStr[18];
        sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
                bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
        return String(macStr);
    }
};

struct KivaStation {
    uint8_t mac[6];
    int8_t rssi;
    uint16_t ap_index; // Index into an AP list if associated
    bool selected;
    // Add other fields as needed

    KivaStation() : rssi(0), ap_index(0xFFFF), selected(false) {
        memset(mac, 0, sizeof(mac));
    }

    String macToString() const {
        char macStr[18];
        sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        return String(macStr);
    }
};

struct KivaSsidInfo { // For beacon list attacks etc.
    String essid;
    uint8_t channel;
    uint8_t bssid[6]; // Optional, can be randomized
    bool selected;

    KivaSsidInfo() : channel(0), selected(false) {
        memset(bssid, 0, sizeof(bssid));
    }
};

// Define constants for max items if using LinkedLists with a practical limit,
// or rely purely on PSRAM capacity. For UI display, limits are still useful.
#define MAX_DISPLAY_APS 50
#define MAX_DISPLAY_STATIONS 50


#endif // RF_TOOLS_COMMON_H