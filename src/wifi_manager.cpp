#include "wifi_manager.h"
#include "sd_card_manager.h" // For SD card operations
#include <Arduino.h>
#include <WiFi.h>
#include <cstring>
#include <algorithm> // For std::sort

// Definition for the global variable
String currentConnectedSsid = "";

// New global from KivaMain.ino, now used here for control
extern bool wifiHardwareEnabled; 

// Static variable to store the current connection attempt status and message
static WifiConnectionStatus currentAttemptStatus = WIFI_STATUS_IDLE;
static unsigned long connectionStartTime = 0;
const unsigned long WIFI_CONNECTION_TIMEOUT_MS = 15000; // 15 seconds timeout
static char wifiStatusString[50] = "Idle";

// Definitions for known networks list
KnownWifiNetwork knownNetworksList[MAX_KNOWN_WIFI_NETWORKS];
int knownNetworksCount = 0;

void setWifiHardwareState(bool enable) {
    if (enable && !wifiHardwareEnabled) {
        Serial.println("Wi-Fi Manager: Enabling Wi-Fi hardware.");
        WiFi.mode(WIFI_STA); // Initialize STA mode
        // WiFi.persistent(false); // Optional: don't save config to NVM by default
        // WiFi.setAutoConnect(false); // We manage connections explicitly
        // WiFi.setAutoReconnect(false);
        wifiHardwareEnabled = true;
        // Note: This doesn't start scanning or connecting, just enables the hardware.
    } else if (!enable && wifiHardwareEnabled) {
        Serial.println("Wi-Fi Manager: Disabling Wi-Fi hardware.");
        WiFi.disconnect(true); // Disconnect and shut down RF
        WiFi.mode(WIFI_OFF);   // Completely turn off Wi-Fi
        wifiHardwareEnabled = false;
        currentConnectedSsid = ""; // Clear any displayed connection
        currentAttemptStatus = WIFI_STATUS_IDLE; // Reset status
        strncpy(wifiStatusString, "Wi-Fi Off", sizeof(wifiStatusString)-1);
        wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
    }
}


bool loadKnownWifiNetworks() {
    if (!isSdCardAvailable()) {
        Serial.println("WiFi Creds Load: SD card not available. Cannot load known networks.");
        knownNetworksCount = 0;
        return false;
    }
    Serial.println("WiFi Creds Load: Attempting to load known networks from SD.");
    String fileContent = readSdFile(KNOWN_WIFI_FILE_PATH); // readSdFile logs its own success/failure
    
    if (fileContent.length() == 0) {
        // This can mean file doesn't exist, is empty, or readSdFile failed.
        // readSdFile already prints "Failed to open file for reading" if SD.open fails.
        Serial.println("WiFi Creds Load: Known networks file is empty or could not be read.");
        knownNetworksCount = 0;
        return true; // Not an error condition for the logic, just means no networks loaded.
    }

    knownNetworksCount = 0;
    int currentIndex = 0;
    int lineStart = 0;
    int linesProcessed = 0;

    while (currentIndex < fileContent.length() && knownNetworksCount < MAX_KNOWN_WIFI_NETWORKS) {
        int ssidEnd = fileContent.indexOf(';', lineStart);
        if (ssidEnd == -1) {
            Serial.printf("WiFi Creds Load: Malformed line %d (missing SSID terminator) at index %d. Stopping parse.\n", linesProcessed + 1, lineStart);
            break; 
        }

        int passEnd = fileContent.indexOf(';', ssidEnd + 1);
        if (passEnd == -1) {
             Serial.printf("WiFi Creds Load: Malformed line %d (missing Password terminator) after SSID '%s'. Stopping parse.\n", linesProcessed + 1, fileContent.substring(lineStart, ssidEnd).c_str());
            break;
        }

        int failCountEnd = fileContent.indexOf('\n', passEnd + 1);
        if (failCountEnd == -1 && (passEnd +1 < fileContent.length())) { // If not the absolute end of file, expect newline
             failCountEnd = fileContent.length(); // Treat as last line
             Serial.printf("WiFi Creds Load: Line %d missing newline, assuming end of file.\n", linesProcessed + 1);
        } else if (failCountEnd == -1) { // Reached end of content properly
             failCountEnd = fileContent.length();
        }


        String ssidStr = fileContent.substring(lineStart, ssidEnd);
        String passStr = fileContent.substring(ssidEnd + 1, passEnd);
        String failStr = fileContent.substring(passEnd + 1, failCountEnd);
        failStr.trim(); // Remove potential \r or spaces

        if (ssidStr.length() == 0 || ssidStr.length() > 32) {
            Serial.printf("WiFi Creds Load: Invalid SSID length on line %d. Skipping.\n", linesProcessed + 1);
            lineStart = failCountEnd + 1;
            linesProcessed++;
            if (lineStart >= fileContent.length()) break;
            continue;
        }


        strncpy(knownNetworksList[knownNetworksCount].ssid, ssidStr.c_str(), sizeof(knownNetworksList[knownNetworksCount].ssid) - 1);
        knownNetworksList[knownNetworksCount].ssid[sizeof(knownNetworksList[knownNetworksCount].ssid) - 1] = '\0';

        strncpy(knownNetworksList[knownNetworksCount].password, passStr.c_str(), sizeof(knownNetworksList[knownNetworksCount].password) - 1);
        knownNetworksList[knownNetworksCount].password[sizeof(knownNetworksList[knownNetworksCount].password) - 1] = '\0';
        
        knownNetworksList[knownNetworksCount].failCount = failStr.toInt();
        if (failStr.toInt() == 0 && failStr != "0") { // Check if toInt failed (e.g. non-numeric string)
            Serial.printf("WiFi Creds Load: Warning - could not parse fail count '%s' for SSID '%s' on line %d. Defaulting to 0.\n", failStr.c_str(), ssidStr.c_str(), linesProcessed + 1);
            knownNetworksList[knownNetworksCount].failCount = 0;
        }


        // Serial.printf("Loaded WiFi: SSID=%s, Pass=*****, Fails=%d\n",
        //               knownNetworksList[knownNetworksCount].ssid,
        //               knownNetworksList[knownNetworksCount].failCount);

        knownNetworksCount++;
        linesProcessed++;
        lineStart = failCountEnd + 1;
        if (lineStart >= fileContent.length()) break;
    }
    Serial.printf("WiFi Creds Load: Successfully parsed and loaded %d known networks from %d lines.\n", knownNetworksCount, linesProcessed);
    return true;
}

bool saveKnownWifiNetworks() {
    if (!isSdCardAvailable()) {
        Serial.println("WiFi Creds Save: SD card not available. CANNOT SAVE known networks.");
        return false;
    }
    if (knownNetworksCount == 0) {
        Serial.println("WiFi Creds Save: No known networks in list to save. Ensuring file is empty or deleted.");
        // Optionally delete the file if it exists and count is 0
        // if (SD.exists(KNOWN_WIFI_FILE_PATH)) {
        //     deleteSdFile(KNOWN_WIFI_FILE_PATH);
        // }
        // Or write an empty string to clear it:
        if (writeSdFile(KNOWN_WIFI_FILE_PATH, "")) {
             Serial.println("WiFi Creds Save: Emptied known networks file.");
             return true;
        } else {
             Serial.println("WiFi Creds Save: Failed to empty known networks file.");
             return false;
        }
    }

    String fileContent = "";
    for (int i = 0; i < knownNetworksCount; i++) {
        // Basic validation before saving
        if (strlen(knownNetworksList[i].ssid) == 0) {
            Serial.printf("WiFi Creds Save: Skipping network at index %d due to empty SSID.\n", i);
            continue;
        }
        fileContent += String(knownNetworksList[i].ssid) + ";";
        fileContent += String(knownNetworksList[i].password) + ";"; // Password can be empty for open networks
        fileContent += String(knownNetworksList[i].failCount) + "\n";
    }

    Serial.printf("WiFi Creds Save: Attempting to save %d known networks to SD.\n", knownNetworksCount);
    if (writeSdFile(KNOWN_WIFI_FILE_PATH, fileContent.c_str())) { // writeSdFile logs its own success/failure
        Serial.printf("WiFi Creds Save: Successfully saved %d known networks.\n", knownNetworksCount);
        return true;
    } else {
        Serial.println("WiFi Creds Save: FAILED to save known networks file.");
        return false;
    }
}

KnownWifiNetwork* findKnownNetwork(const char* ssid) {
    for (int i = 0; i < knownNetworksCount; i++) {
        if (strcmp(knownNetworksList[i].ssid, ssid) == 0) {
            return &knownNetworksList[i];
        }
    }
    return nullptr;
}

bool addOrUpdateKnownNetwork(const char* ssid, const char* password, bool resetFailCountOnUpdate) {
    if (strlen(ssid) == 0) {
        Serial.println("WiFi Creds Add/Update: Attempted to add/update with an empty SSID. Aborting.");
        return false;
    }
    // Password can be empty for open networks.

    KnownWifiNetwork* existingNetwork = findKnownNetwork(ssid);
    bool changed = false;

    if (existingNetwork) {
        // Update existing
        if (strcmp(existingNetwork->password, password) != 0) {
            strncpy(existingNetwork->password, password, sizeof(existingNetwork->password) - 1);
            existingNetwork->password[sizeof(existingNetwork->password) - 1] = '\0';
            changed = true;
        }
        if (resetFailCountOnUpdate && existingNetwork->failCount != 0) {
            existingNetwork->failCount = 0;
            changed = true;
        }
        if (changed) {
            Serial.printf("WiFi Creds Add/Update: Updated network %s. ResetFail: %d\n", ssid, resetFailCountOnUpdate);
        } else {
            // Serial.printf("WiFi Creds Add/Update: Network %s already up-to-date.\n", ssid);
        }
    } else {
        // Add new, if space available
        if (knownNetworksCount < MAX_KNOWN_WIFI_NETWORKS) {
            strncpy(knownNetworksList[knownNetworksCount].ssid, ssid, sizeof(knownNetworksList[knownNetworksCount].ssid) - 1);
            knownNetworksList[knownNetworksCount].ssid[sizeof(knownNetworksList[knownNetworksCount].ssid) - 1] = '\0';
            
            strncpy(knownNetworksList[knownNetworksCount].password, password, sizeof(knownNetworksList[knownNetworksCount].password) - 1);
            knownNetworksList[knownNetworksCount].password[sizeof(knownNetworksList[knownNetworksCount].password) - 1] = '\0';
            
            knownNetworksList[knownNetworksCount].failCount = 0; // New networks start with 0 fails
            knownNetworksCount++;
            changed = true;
            Serial.printf("WiFi Creds Add/Update: Added new network %s.\n", ssid);
        } else {
            Serial.println("WiFi Creds Add/Update: Max known networks reached. Cannot add new one.");
            return false; 
        }
    }

    if (changed) {
        return saveKnownWifiNetworks();
    }
    return true; // No changes needed, so effectively success.
}

bool incrementSsidFailCount(const char* ssid) {
    if (strlen(ssid) == 0) return false;
    KnownWifiNetwork* network = findKnownNetwork(ssid);
    if (network) {
        network->failCount++;
        Serial.printf("WiFi Creds FailCount: Incremented fail count for %s to %d.\n", ssid, network->failCount);
        return saveKnownWifiNetworks();
    }
    Serial.printf("WiFi Creds FailCount: Could not find network %s to increment fail count.\n", ssid);
    return false;
}

bool resetSsidFailCount(const char* ssid) {
    if (strlen(ssid) == 0) return false;
    KnownWifiNetwork* network = findKnownNetwork(ssid);
    if (network) {
        if (network->failCount != 0) { 
            network->failCount = 0;
            Serial.printf("WiFi Creds FailCount: Reset fail count for %s.\n", ssid);
            return saveKnownWifiNetworks();
        }
        // Serial.printf("WiFi Creds FailCount: Fail count for %s already 0.\n", ssid);
        return true; 
    }
    Serial.printf("WiFi Creds FailCount: Could not find network %s to reset fail count.\n", ssid);
    return false;
}


void setupWifi() {
    // Initial state is off by default (wifiHardwareEnabled = false in KivaMain.ino)
    // setWifiHardwareState(false); // Explicitly ensure it's off if default changes
    
    // Load known networks regardless of initial hardware state.
    // They are loaded into RAM for when Wi-Fi is turned on.
    if (isSdCardAvailable()) {
        loadKnownWifiNetworks();
    } else {
        Serial.println("Wi-Fi Manager: SD Card not available, cannot load known Wi-Fi networks.");
        knownNetworksCount = 0;
    }
    // Do not initialize WiFi.mode(WIFI_STA) here anymore.
    // It will be done by setWifiHardwareState(true) when Wi-Fi is needed.
    Serial.println("Wi-Fi Manager: Setup complete. Wi-Fi hardware initially off.");
    strncpy(wifiStatusString, "Wi-Fi Off", sizeof(wifiStatusString)-1);
    wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
}

int initiateAsyncWifiScan() {
    if (!wifiHardwareEnabled) {
        Serial.println("Wi-Fi Manager: Cannot scan, Wi-Fi hardware is disabled.");
        // Optionally update status string to "Wi-Fi Disabled" for UI
        strncpy(wifiStatusString, "Wi-Fi Off", sizeof(wifiStatusString)-1);
        wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
        foundWifiNetworksCount = 0; // No networks if Wi-Fi is off
        return -3; // Custom code for Wi-Fi disabled, or handle in caller
    }

    Serial.println("Wi-Fi Manager: Initiating async scan...");
    WiFi.scanDelete(); 
    delay(50); 
    int scanResult = WiFi.scanNetworks(true, true); 

    if (scanResult == WIFI_SCAN_FAILED) { 
        Serial.println("Wi-Fi Manager: Scan initiation failed.");
        currentAttemptStatus = KIVA_WIFI_SCAN_FAILED;
        strncpy(wifiStatusString, "Scan Failed", sizeof(wifiStatusString)-1);
    } else if (scanResult == WIFI_SCAN_RUNNING) { 
        Serial.println("Wi-Fi Manager: Scan started successfully, running in background.");
        currentAttemptStatus = KIVA_WIFI_SCAN_RUNNING;
        strncpy(wifiStatusString, "Scanning...", sizeof(wifiStatusString)-1);
    } else { // Positive number means synchronous scan completed (shouldn't happen with async=true)
             // or other unexpected value.
        Serial.printf("Wi-Fi Manager: scanNetworks call returned unexpected for async: %d\n", scanResult);
        // Treat as if scan is done or failed to start async properly.
        // Caller will call checkAndRetrieveWifiScanResults which will handle it.
        currentAttemptStatus = WIFI_STATUS_IDLE; // Or some error
        strncpy(wifiStatusString, "Scan Done?", sizeof(wifiStatusString)-1); 
    }
    wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
    return scanResult; 
}

int checkAndRetrieveWifiScanResults() {
    int scanStatus = WiFi.scanComplete(); 

    if (scanStatus == WIFI_SCAN_FAILED) { // -2 from ESP32 WiFi library
        Serial.println("Wi-Fi Manager: WiFi.scanComplete() reported scan FAILED.");
        foundWifiNetworksCount = 0;
        currentAttemptStatus = KIVA_WIFI_SCAN_FAILED; // Use our enum
        strncpy(wifiStatusString, "Scan Failed", sizeof(wifiStatusString)-1);
        wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
        WiFi.scanDelete(); // Clean up results from WiFi library
        return KIVA_WIFI_SCAN_FAILED; // Return our status
    } else if (scanStatus == WIFI_SCAN_RUNNING) { // -1 from ESP32 WiFi library
        // Still scanning, do nothing more here.
        // Serial.println("Wi-Fi Manager: WiFi.scanComplete() reports scan STILL RUNNING.");
        currentAttemptStatus = KIVA_WIFI_SCAN_RUNNING; // Use our enum
        // wifiStatusString should already be "Scanning..."
        return KIVA_WIFI_SCAN_RUNNING; // Return our status
    } else if (scanStatus >= 0) { // Scan finished (found 'scanStatus' networks or 0)
        Serial.print("Wi-Fi Manager: Async Scan done, ");
        Serial.print(scanStatus); 
        Serial.println(" networks found by WiFi.scanComplete().");

        // Temporary array to hold all results from the scan before filtering and sorting
        WifiNetwork tempScannedNetworks[MAX_WIFI_NETWORKS]; 
        int tempCount = 0; // Count of networks in tempScannedNetworks

        // Populate tempScannedNetworks from WiFi library results
        if (scanStatus > 0) { // Only process if networks were actually found by the scan
            for (int i = 0; i < scanStatus && tempCount < MAX_WIFI_NETWORKS; ++i) {
                String ssidStr = WiFi.SSID(i);
                ssidStr.trim(); // Remove leading/trailing whitespace
                if (ssidStr.length() == 0 || ssidStr.length() > 32) { // Skip empty or overly long SSIDs
                    // Serial.printf("Skipping invalid SSID from scan: '%s'\n", WiFi.SSID(i).c_str());
                    continue; // Move to the next scanned network
                }

                strncpy(tempScannedNetworks[tempCount].ssid, ssidStr.c_str(), 32);
                tempScannedNetworks[tempCount].ssid[32] = '\0'; // Ensure null termination
                tempScannedNetworks[tempCount].rssi = WiFi.RSSI(i);
                tempScannedNetworks[tempCount].isSecure = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
                tempCount++;
            }
        }
        
        // Sort all valid networks found in tempScannedNetworks by RSSI (strongest first)
        std::sort(tempScannedNetworks, tempScannedNetworks + tempCount, [](const WifiNetwork& a, const WifiNetwork& b) {
            return a.rssi > b.rssi; // Sort for strongest RSSI first
        });

        // Now, populate the final `scannedNetworks` list for the UI,
        // prioritizing the currently connected network (if any) at the top.
        foundWifiNetworksCount = 0; // Reset the count for the final list
        int connectedNetworkOriginalIndexInTemp = -1; // To track if connected network was in temp and avoid re-adding

        // If currently connected to a Wi-Fi, try to find it in the temp sorted list and add it first
        if (currentConnectedSsid.length() > 0) {
            for (int i = 0; i < tempCount; ++i) {
                if (currentConnectedSsid.equals(tempScannedNetworks[i].ssid)) {
                    if (foundWifiNetworksCount < MAX_WIFI_NETWORKS) {
                        scannedNetworks[foundWifiNetworksCount++] = tempScannedNetworks[i];
                        connectedNetworkOriginalIndexInTemp = i; // Mark it so we don't add it again from the loop below
                    }
                    break; // Found the connected network, no need to search further in temp list for it
                }
            }
        }

        // Add the rest of the networks from the sorted temporary list,
        // skipping the one that was already added (if it was the connected one).
        for (int i = 0; i < tempCount; ++i) {
            if (i == connectedNetworkOriginalIndexInTemp) {
                continue; // Skip if it was the connected one and already added to `scannedNetworks`
            }
            if (foundWifiNetworksCount < MAX_WIFI_NETWORKS) { // Ensure we don't overflow `scannedNetworks`
                scannedNetworks[foundWifiNetworksCount++] = tempScannedNetworks[i];
            } else {
                break; // `scannedNetworks` is full
            }
        }

        // Update status string based on results
        if (foundWifiNetworksCount == 0) {
             Serial.println("Wi-Fi Manager: No valid networks stored after filtering and sorting.");
             strncpy(wifiStatusString, "No Networks", sizeof(wifiStatusString)-1);
        } else {
             snprintf(wifiStatusString, sizeof(wifiStatusString)-1, "%d Networks", foundWifiNetworksCount);
        }
        wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
        currentAttemptStatus = WIFI_STATUS_IDLE; // Scan is no longer running or failed, it's idle/done for now
        
        WiFi.scanDelete(); // Clean up scan results from WiFi library internal storage
        return foundWifiNetworksCount; // Return actual count of networks in `scannedNetworks`
    }
    
    // This part should ideally not be reached if the above conditions for scanStatus are exhaustive.
    Serial.printf("Wi-Fi Manager: WiFi.scanComplete() returned unhandled status: %d\n", scanStatus);
    currentAttemptStatus = WIFI_STATUS_IDLE; // Or some other appropriate error state
    strncpy(wifiStatusString, "Scan Unknown", sizeof(wifiStatusString)-1);
    wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
    WiFi.scanDelete(); // Clean up in case of unexpected status too
    return scanStatus; // Return the raw status from WiFi.scanComplete()
}


wl_status_t getCurrentWifiStatus() {
    return WiFi.status();
}

String getCurrentSsid() {
    if (WiFi.status() == WL_CONNECTED) {
        currentConnectedSsid = WiFi.SSID(); // Keep our global updated
        return currentConnectedSsid;
    }
    // If not connected, currentConnectedSsid will retain its last connected value or be empty.
    // It's cleared during new connection attempts or on failure.
    return ""; // Return empty if not actively connected at this moment
}

void attemptDirectWifiConnection(const char* ssid) {
    if (!wifiHardwareEnabled) {
        Serial.println("Attempting direct connection but Wi-Fi is off. Enabling first.");
        setWifiHardwareState(true); // Ensure Wi-Fi is on
        delay(100); // Give it a moment to power up
    }

    Serial.printf("Wi-Fi Manager: Attempting to connect to OPEN network: %s\n", ssid);
    currentConnectedSsid = ""; 
    WiFi.disconnect(); 
    delay(100);
    WiFi.begin(ssid); // For open networks, password is NULL by default
    connectionStartTime = millis();
    currentAttemptStatus = WIFI_CONNECTING_IN_PROGRESS;
    strncpy(wifiStatusString, "Connecting...", sizeof(wifiStatusString)-1);
    wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
    // For open networks, we can "confirm" the password as empty immediately
    addOrUpdateKnownNetwork(ssid, "", true);
}

void attemptWpaWifiConnection(const char* ssid, const char* password) {
    if (!wifiHardwareEnabled) {
        Serial.println("Attempting WPA connection but Wi-Fi is off. Enabling first.");
        setWifiHardwareState(true); // Ensure Wi-Fi is on
        delay(100); // Give it a moment to power up
    }

    Serial.printf("Wi-Fi Manager: Attempting to connect to SECURE network: %s (Pass: ***)\n", ssid);
    currentConnectedSsid = ""; 
    WiFi.disconnect(); 
    delay(100);
    WiFi.begin(ssid, password);
    connectionStartTime = millis();
    currentAttemptStatus = WIFI_CONNECTING_IN_PROGRESS;
    strncpy(wifiStatusString, "Connecting...", sizeof(wifiStatusString)-1);
    wifiStatusString[sizeof(wifiStatusString)-1] = '\0';
    // Note: We save/update the password more definitively upon successful connection or explicit input.
    // If this attempt is with a *new* password from user input, it will be saved by handleKeyboardInput.
    // If this attempt is with a *stored* password, success/failure will update fail counts.
}

WifiConnectionStatus checkWifiConnectionProgress() {
    if (currentAttemptStatus != WIFI_CONNECTING_IN_PROGRESS) {
        return currentAttemptStatus; 
    }

    wl_status_t status = WiFi.status();
    String connectingSsid = WiFi.SSID(); // Get SSID of current attempt if available

    if (status == WL_CONNECTED) {
        Serial.println("Wi-Fi Manager: Connected successfully!");
        currentAttemptStatus = WIFI_CONNECTED_SUCCESS;
        currentConnectedSsid = WiFi.SSID(); // Update global here
        snprintf(wifiStatusString, sizeof(wifiStatusString)-1, "Connected: %s", currentConnectedSsid.c_str());
        // On successful connection, reset fail count for this SSID.
        // The password used for this successful connection is now considered "good".
        // If currentSsidToConnect was set (e.g. from selection or input), use that.
        // Otherwise, use WiFi.SSID().
        const char* ssidToUpdate = (strlen(currentSsidToConnect) > 0) ? currentSsidToConnect : currentConnectedSsid.c_str();
        if (strlen(ssidToUpdate) > 0) {
            resetSsidFailCount(ssidToUpdate);
            // If it was a secure network and we have the password (e.g. from wifiPasswordInput), ensure it's saved.
            // This logic is tricky because we don't always have the password here if it was a stored one.
            // addOrUpdateKnownNetwork in handleKeyboardInput and upon connection success handles password saving.
        }
    } else if (status == WL_NO_SSID_AVAIL) {
        Serial.println("Wi-Fi Manager: Connection Failed - SSID not available.");
        currentAttemptStatus = WIFI_FAILED_NO_SSID_AVAIL;
        strncpy(wifiStatusString, "SSID Not Found", sizeof(wifiStatusString)-1);
        currentConnectedSsid = ""; 
    } else if (status == WL_CONNECT_FAILED) { // Often means wrong password for WPA
        Serial.println("Wi-Fi Manager: Connection Failed - WL_CONNECT_FAILED.");
        const char* ssidOfFailedAttempt = (strlen(currentSsidToConnect) > 0) ? currentSsidToConnect : connectingSsid.c_str();

        if (selectedNetworkIsSecure && strlen(ssidOfFailedAttempt) > 0) {
            currentAttemptStatus = WIFI_FAILED_WRONG_PASSWORD;
            strncpy(wifiStatusString, "Auth Error", sizeof(wifiStatusString)-1);
            incrementSsidFailCount(ssidOfFailedAttempt);
        } else {
            currentAttemptStatus = WIFI_FAILED_OTHER; 
            strncpy(wifiStatusString, "Connect Fail", sizeof(wifiStatusString)-1);
        }
        currentConnectedSsid = ""; 
    } else if (status == WL_IDLE_STATUS || status == WL_DISCONNECTED || status == WL_SCAN_COMPLETED) {
        if (millis() - connectionStartTime > WIFI_CONNECTION_TIMEOUT_MS) {
            Serial.println("Wi-Fi Manager: Connection Timeout.");
            currentAttemptStatus = WIFI_FAILED_TIMEOUT;
            strncpy(wifiStatusString, "Timeout", sizeof(wifiStatusString)-1);
            currentConnectedSsid = ""; 
            // Optionally increment fail count on timeout too if currentSsidToConnect is set
            if (strlen(currentSsidToConnect) > 0) {
                 incrementSsidFailCount(currentSsidToConnect);
            }
            WiFi.disconnect(); 
        } else {
            // Still trying, status is just idle or disconnected but not yet timed out.
            currentAttemptStatus = WIFI_CONNECTING_IN_PROGRESS;
        }
    } else { // Other statuses like WL_NO_SHIELD, WL_CONNECTION_LOST etc.
        if (millis() - connectionStartTime > WIFI_CONNECTION_TIMEOUT_MS) {
            Serial.printf("Wi-Fi Manager: Timeout with unhandled status %d.\n", status);
            currentAttemptStatus = WIFI_FAILED_TIMEOUT; // Or WIFI_FAILED_OTHER
            strncpy(wifiStatusString, "Timeout/Error", sizeof(wifiStatusString)-1);
            currentConnectedSsid = ""; 
            if (strlen(currentSsidToConnect) > 0) {
                 incrementSsidFailCount(currentSsidToConnect);
            }
            WiFi.disconnect();
        } else {
            currentAttemptStatus = WIFI_CONNECTING_IN_PROGRESS;
        }
    }
    wifiStatusString[sizeof(wifiStatusString)-1] = '\0'; 
    return currentAttemptStatus;
}


WifiConnectionStatus getCurrentWifiConnectionStatus() {
    return currentAttemptStatus;
}

const char* getWifiStatusMessage() {
    return wifiStatusString;
}