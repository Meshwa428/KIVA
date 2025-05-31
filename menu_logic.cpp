// menu_logic.cpp
#include "menu_logic.h"
#include "config.h" 
#include "ui_drawing.h" 
#include "wifi_manager.h" 

const char* mainMenuItems[] = { "Tools", "Games", "Settings", "Info" }; 
const char* gamesMenuItems[] = { "Snake Game", "Tetris Clone", "Classic Pong", "2D Maze", "Back" }; 
const char* settingsMenuItems[] = { "Wi-Fi Setup", "Display Opts", "Sound Setup", "System Info", "About Kiva", "Back" }; 
const char* toolsMenuItems[] = { "Injection", "Wi-Fi Attacks", "BLE/BT Attacks", "NRF Recon", "Jamming", "Back" }; 
const char* injectionToolItems[] = { "MouseJack", "Wireless Keystroke", "Fake HID", "BLE HID Inject", "RF Payload", "Back" }; 
const char* wifiAttackToolItems[] = { "Beacon Spam", "Deauth Attack", "Probe Flood", "Fake AP", "Karma AP", "DNS Spoof", "Packet Inject", "Packet Replay", "Back" }; 
const char* bleAttackToolItems[] = { "BLE Ad Spam", "BLE Scanner", "BLE DoS", "BLE Name Spoof", "BT MAC Spoof", "Back" }; 
const char* nrfReconToolItems[] = { "NRF Scanner", "ShockBurst Sniff", "Key Sniffer", "NRF Brute", "Custom Flood", "RF Spammer", "BLE Ad Disrupt", "Dual Interference", "Back" }; 
const char* jammingToolItems[] = { "NRF Jam", "Wi-Fi Deauth Jam", "BLE Flood Jam", "Channel Hop Jam", "RF Noise Flood", "Back" }; 

int getMainMenuItemsCount() { return sizeof(mainMenuItems) / sizeof(mainMenuItems[0]); }
int getGamesMenuItemsCount() { return sizeof(gamesMenuItems) / sizeof(gamesMenuItems[0]); }
int getToolsMenuItemsCount() { return sizeof(toolsMenuItems) / sizeof(toolsMenuItems[0]); }
int getSettingsMenuItemsCount() { return sizeof(settingsMenuItems) / sizeof(settingsMenuItems[0]); }
int getInjectionToolItemsCount() { return sizeof(injectionToolItems) / sizeof(injectionToolItems[0]); }
int getWifiAttackToolItemsCount() { return sizeof(wifiAttackToolItems) / sizeof(wifiAttackToolItems[0]); }
int getBleAttackToolItemsCount() { return sizeof(bleAttackToolItems) / sizeof(bleAttackToolItems[0]); }
int getNrfReconToolItemsCount() { return sizeof(nrfReconToolItems) / sizeof(nrfReconToolItems[0]); }
int getJammingToolItemsCount() { return sizeof(jammingToolItems) / sizeof(jammingToolItems[0]); }

void initializeCurrentMenu() {
    marqueeActive = false; 
    marqueeOffset = 0;
    marqueeScrollLeft = true;
    gridAnimatingIn = false; 

    switch (currentMenu) {
        case MAIN_MENU:
            maxMenuItems = getMainMenuItemsCount(); 
            mainMenuAnim.init(); 
            mainMenuAnim.setTargets(menuIndex, maxMenuItems);
            break;
        case GAMES_MENU:
            maxMenuItems = getGamesMenuItemsCount(); 
            subMenuAnim.init();
            subMenuAnim.setTargets(menuIndex, maxMenuItems);
            break;
        case TOOLS_MENU:
            maxMenuItems = getToolsMenuItemsCount(); 
            subMenuAnim.init();
            subMenuAnim.setTargets(menuIndex, maxMenuItems);
            break;
        case SETTINGS_MENU:
            maxMenuItems = getSettingsMenuItemsCount(); 
            subMenuAnim.init();
            subMenuAnim.setTargets(menuIndex, maxMenuItems);
            break;
        case TOOL_CATEGORY_GRID:
            switch (toolsCategoryIndex) { 
                case 0: maxMenuItems = getInjectionToolItemsCount(); break; 
                case 1: maxMenuItems = getWifiAttackToolItemsCount(); break; 
                case 2: maxMenuItems = getBleAttackToolItemsCount(); break;   
                case 3: maxMenuItems = getNrfReconToolItemsCount(); break;  
                case 4: maxMenuItems = getJammingToolItemsCount(); break;   
                default: maxMenuItems = 0; break; 
            }
            targetGridScrollOffset_Y = 0; 
            currentGridScrollOffset_Y_anim = 0;
            startGridItemAnimation(); 
            break;
        case WIFI_SETUP_MENU: 
            if (wifiIsScanning) { // global wifiIsScanning
                maxMenuItems = 1; // Just show "Scan Again" or "Back" effectively during scan.
                                 // Or perhaps just a "Cancel Scan" option becomes item 0.
                                 // Let's assume for now it shows "Scanning..." and back button works.
                                 // For simplicity, while scanning, list might not be interactive beyond back.
                                 // So maxMenuItems should be 0 to prevent interaction if just showing "Scanning..."
                                 // Or set it to 1 if "Back" is the only item to select.
                                 // Let's adjust to just show the "Scanning..." text, so list anim not critical
                maxMenuItems = 0; // No selectable items in list view during scan message
            } else {
                maxMenuItems = foundWifiNetworksCount + 2; // Networks + "Scan Again" + "Back"
            }
            wifiListAnim.init();
            // Use wifiMenuIndex when in WIFI_SETUP_MENU
            // Ensure wifiMenuIndex is clamped if maxMenuItems changes due to scan finishing
            if (wifiMenuIndex >= maxMenuItems && maxMenuItems > 0) wifiMenuIndex = maxMenuItems -1;
            else if (maxMenuItems == 0) wifiMenuIndex = 0;

            wifiListAnim.setTargets(wifiMenuIndex, maxMenuItems); 
            break;
    }
    // Clamp generic menuIndex (used by most menus)
    if (currentMenu != WIFI_SETUP_MENU) { 
        if (menuIndex >= maxMenuItems && maxMenuItems > 0) { 
            menuIndex = maxMenuItems -1;
        } else if (maxMenuItems == 0 && menuIndex != 0) { 
            menuIndex = 0;
        }
    }
}

void handleMenuSelection() {
    MenuState previousMenu = currentMenu; 
    int localMenuIndexForNewMenu = 0; // Default starting index for new menus

    if (currentMenu == MAIN_MENU) {
        mainMenuSavedIndex = menuIndex; // Save the selection from the main menu
        MenuState targetMenu = currentMenu;

        if (menuIndex == 0) { targetMenu = TOOLS_MENU; }
        else if (menuIndex == 1) { targetMenu = GAMES_MENU; } 
        else if (menuIndex == 2) { targetMenu = SETTINGS_MENU; } 
        else if (menuIndex == 3) { /* Info selected - no state change, no re-init needed */ return; }
        
        if (currentMenu != targetMenu) { 
            currentMenu = targetMenu;
            menuIndex = localMenuIndexForNewMenu; // Explicitly use 0 for new submenus
            initializeCurrentMenu();
        }
    } else if (currentMenu == TOOLS_MENU) {
        int currentMax = getToolsMenuItemsCount();
        if (menuIndex == (currentMax - 1)) { // "Back"
            currentMenu = MAIN_MENU; 
            menuIndex = mainMenuSavedIndex; // Restore previous main menu index
        } else { 
            toolsCategoryIndex = menuIndex; 
            currentMenu = TOOL_CATEGORY_GRID; 
            menuIndex = localMenuIndexForNewMenu; // Start grid at 0
        }
        initializeCurrentMenu();
    } else if (currentMenu == GAMES_MENU) {
        int currentMax = getGamesMenuItemsCount();
        if (menuIndex == (currentMax - 1)) { // "Back"
            currentMenu = MAIN_MENU; 
            menuIndex = mainMenuSavedIndex;
        } else { 
            Serial.printf("Game selected: %s\n", gamesMenuItems[menuIndex]);
            // Actual game launch logic would go here, possibly changing currentMenu
        }
        // If launching a game changes currentMenu, init will be called by that logic.
        // If just selecting and staying, but an action happens, an explicit re-init might not be needed
        // unless the visual state of the menu itself needs to reset (which it doesn't here).
        // For now, assume if not "Back", we stay on GAMES_MENU unless game logic changes state.
        // If menu changes, then init:
        if (previousMenu != currentMenu) initializeCurrentMenu();

    } else if (currentMenu == SETTINGS_MENU) {
        int currentMax = getSettingsMenuItemsCount();
        if (menuIndex == (currentMax - 1)) { // "Back"
            currentMenu = MAIN_MENU; 
            menuIndex = mainMenuSavedIndex;
        } else if (strcmp(settingsMenuItems[menuIndex], "Wi-Fi Setup") == 0) { 
            currentMenu = WIFI_SETUP_MENU;
            wifiMenuIndex = localMenuIndexForNewMenu; // Start Wi-Fi list at 0
            // Assuming initiateAsyncWifiScan is still desired here from previous logic
            int scanReturn = initiateAsyncWifiScan(); 
            if (scanReturn == WIFI_SCAN_RUNNING) {
                wifiIsScanning = true; foundWifiNetworksCount = 0; 
                lastWifiScanCheckTime = millis();
            } else if (scanReturn >= 0) { 
                wifiIsScanning = false; checkAndRetrieveWifiScanResults(); 
            } else { 
                wifiIsScanning = false; Serial.println("Failed to start Wi-Fi scan."); foundWifiNetworksCount = 0;
            }
        } else { 
            Serial.printf("Setting selected: %s\n", settingsMenuItems[menuIndex]);
            // Actual setting change logic
        }
        // If menu changes, then init:
        if (previousMenu != currentMenu) initializeCurrentMenu();

    } else if (currentMenu == TOOL_CATEGORY_GRID) {
        int currentMax = 0;
        switch(toolsCategoryIndex) { // Determine max for current tool grid
            case 0: currentMax = getInjectionToolItemsCount(); break;
            case 1: currentMax = getWifiAttackToolItemsCount(); break;
            case 2: currentMax = getBleAttackToolItemsCount(); break;
            case 3: currentMax = getNrfReconToolItemsCount(); break;
            case 4: currentMax = getJammingToolItemsCount(); break;
        }
        if (menuIndex == currentMax - 1 && currentMax > 0) { // "Back"
            currentMenu = TOOLS_MENU; 
            menuIndex = toolsCategoryIndex; // Select the category we came from
        } else if (currentMax > 0) { 
             const char** currentToolList = nullptr; // Get the actual tool text
             switch(toolsCategoryIndex){ /* assign currentToolList based on toolsCategoryIndex */ }
             // Serial.printf("Tool selected: %s\n", currentToolList[menuIndex]);
        }
        initializeCurrentMenu(); // Always re-init if selection was made or backed out
    } else if (currentMenu == WIFI_SETUP_MENU) {
        if (!wifiIsScanning) {
            if (wifiMenuIndex < foundWifiNetworksCount) {
                Serial.print("Selected WiFi to connect: "); Serial.println(scannedNetworks[wifiMenuIndex].ssid);
                // TODO: Implement password entry / connection logic
            } else if (wifiMenuIndex == foundWifiNetworksCount) { // "Scan Again"
                Serial.println("Re-initiating Wi-Fi scan...");
                int scanReturn = initiateAsyncWifiScan();
                if (scanReturn == WIFI_SCAN_RUNNING) {
                    wifiIsScanning = true; foundWifiNetworksCount = 0; wifiMenuIndex = 0; 
                    lastWifiScanCheckTime = millis();
                } else if (scanReturn >= 0) {
                    wifiIsScanning = false; checkAndRetrieveWifiScanResults(); wifiMenuIndex = 0;
                } else {
                    wifiIsScanning = false; Serial.println("Failed to start re-scan.");
                }
                initializeCurrentMenu(); // This will update maxMenuItems for wifi setup and retarget animation
            } else if (wifiMenuIndex == foundWifiNetworksCount + 1) { // "Back"
                currentMenu = SETTINGS_MENU;
                // Find "Wi-Fi Setup" in settings to pre-select it
                bool foundWifiSetup = false;
                for(int i=0; i < getSettingsMenuItemsCount(); ++i) {
                    if (strcmp(settingsMenuItems[i], "Wi-Fi Setup") == 0) {
                        menuIndex = i; // Use generic menuIndex for settings menu
                        foundWifiSetup = true;
                        break;
                    }
                }
                if (!foundWifiSetup) menuIndex = 0;
                initializeCurrentMenu();
            }
        }
    }
}

void handleMenuBackNavigation() {
    MenuState previousMenuState = currentMenu; 

    if (currentMenu == MAIN_MENU) {
        return; 
    } else if (currentMenu == TOOL_CATEGORY_GRID) {
        currentMenu = TOOLS_MENU;
        menuIndex = toolsCategoryIndex; 
    } else if (currentMenu == WIFI_SETUP_MENU) { 
        wifiIsScanning = false; 
        currentMenu = SETTINGS_MENU;
        bool foundWifiSetupItem = false;
        for(int i=0; i < getSettingsMenuItemsCount(); ++i) { 
            if (strcmp(settingsMenuItems[i], "Wi-Fi Setup") == 0) {
                menuIndex = i;
                foundWifiSetupItem = true;
                break;
            }
        }
        if (!foundWifiSetupItem) { 
            menuIndex = 0; 
        }
        
    } else if (currentMenu == GAMES_MENU || currentMenu == TOOLS_MENU || currentMenu == SETTINGS_MENU) {
        currentMenu = MAIN_MENU;
        menuIndex = mainMenuSavedIndex; 
    } 
    
    if (previousMenuState != currentMenu) {
        initializeCurrentMenu(); 
    }
}