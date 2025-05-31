#include "wifi_manager.h"
#include <Arduino.h>    
#include <WiFi.h>       
#include <cstring>      

// setupWifi (as before)
void setupWifi() {
    WiFi.mode(WIFI_STA); 
    WiFi.disconnect(); 
    delay(100);
    Serial.println("Wi-Fi Manager: STA Mode Initialized.");
}

// initiateAsyncWifiScan (as before)
int initiateAsyncWifiScan() {
    Serial.println("Wi-Fi Manager: Initiating async scan...");
    WiFi.scanDelete(); 
    delay(50); 
    return WiFi.scanNetworks(true, true); 
}


int checkAndRetrieveWifiScanResults() {
    int scanStatus = WiFi.scanComplete();

    if (scanStatus >= 0) { // Scan finished (could be 0 networks, or >0)
        Serial.print("Wi-Fi Manager: Async Scan done, ");
        Serial.print(scanStatus);
        Serial.println(" networks found by hardware.");

        foundWifiNetworksCount = 0; // Reset our count of valid, stored networks

        if (scanStatus > 0) {
        for (int i = 0; i < scanStatus && foundWifiNetworksCount < MAX_WIFI_NETWORKS; ++i) {
            String ssidStr = WiFi.SSID(i);
            ssidStr.trim(); 

            if (ssidStr.length() == 0 || ssidStr.length() > 32) { 
                continue; 
            }

            strncpy(scannedNetworks[foundWifiNetworksCount].ssid, ssidStr.c_str(), 32);
            scannedNetworks[foundWifiNetworksCount].ssid[32] = '\0'; 
            
            scannedNetworks[foundWifiNetworksCount].rssi = WiFi.RSSI(i); // THIS IS THE KEY LINE
            // ***** ADD THIS DEBUG LINE *****
            Serial.printf("WiFi Scan: SSID: %s, Raw RSSI: %d\n", scannedNetworks[foundWifiNetworksCount].ssid, scannedNetworks[foundWifiNetworksCount].rssi);
            // ******************************
            scannedNetworks[foundWifiNetworksCount].isSecure = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
            
            foundWifiNetworksCount++;
        }

            // Sort the found (and stored) networks by RSSI
            for (int i = 0; i < foundWifiNetworksCount - 1; i++) {
                for (int j = 0; j < foundWifiNetworksCount - i - 1; j++) {
                    if (scannedNetworks[j].rssi < scannedNetworks[j + 1].rssi) {
                        WifiNetwork temp = scannedNetworks[j];
                        scannedNetworks[j] = scannedNetworks[j + 1];
                        scannedNetworks[j + 1] = temp;
                    }
                }
            }
        }
        
        if (foundWifiNetworksCount == 0) {
             Serial.println("Wi-Fi Manager: No valid networks stored after filtering.");
        }
        // WiFi.scanDelete(); // Clear scan results from WiFi class memory - already called before starting new scan
    }
    // if scanStatus == WIFI_SCAN_RUNNING (-1), this function returns -1
    // if scanStatus == WIFI_SCAN_FAILED (-2), this function returns -2
    return scanStatus; 
}

// getCurrentWifiStatus (as before)
wl_status_t getCurrentWifiStatus() {
    return WiFi.status();
}

// getCurrentSsid (as before)
String getCurrentSsid() {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.SSID();
    }
    return ""; 
}