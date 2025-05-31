// KivaMain/menu_logic.cpp

#include "menu_logic.h"
#include "config.h" 
#include "ui_drawing.h" 
#include "wifi_manager.h"
#include "pcf_utils.h" 

// --- Menu Item Arrays (ensure "Utilities" is in mainMenuItems as configured) ---
const char* mainMenuItems[] = { 
    "Tools", 
    "Games", 
    "Settings", 
    "Utilities", 
    "Info" 
}; 
const char* gamesMenuItems[] = { "Snake Game", "Tetris Clone", "Classic Pong", "2D Maze", "Back" }; 
const char* settingsMenuItems[] = { "Wi-Fi Setup", "Display Opts", "Sound Setup", "System Info", "About Kiva", "Back" }; 
const char* utilitiesMenuItems[] = { "Vibration", "Laser", "Flashlight", "Back" }; 
const char* toolsMenuItems[] = { "Injection", "Wi-Fi Attacks", "BLE/BT Attacks", "NRF Recon", "Jamming", "Back" }; 
const char* injectionToolItems[] = { "MouseJack", "Wireless Keystroke", "Fake HID", "BLE HID Inject", "RF Payload", "Back" }; 
const char* wifiAttackToolItems[] = { "Beacon Spam", "Deauth Attack", "Probe Flood", "Fake AP", "Karma AP", "DNS Spoof", "Packet Inject", "Packet Replay", "Back" }; 
const char* bleAttackToolItems[] = { "BLE Ad Spam", "BLE Scanner", "BLE DoS", "BLE Name Spoof", "BT MAC Spoof", "Back" }; 
const char* nrfReconToolItems[] = { "NRF Scanner", "ShockBurst Sniff", "Key Sniffer", "NRF Brute", "Custom Flood", "RF Spammer", "BLE Ad Disrupt", "Dual Interference", "Back" }; 
const char* jammingToolItems[] = { "NRF Jam", "Wi-Fi Deauth Jam", "BLE Flood Jam", "Channel Hop Jam", "RF Noise Flood", "Back" }; 


// --- Getter Functions for Menu Item Counts ---
int getMainMenuItemsCount() { return sizeof(mainMenuItems) / sizeof(mainMenuItems[0]); }
int getGamesMenuItemsCount() { return sizeof(gamesMenuItems) / sizeof(gamesMenuItems[0]); }
int getToolsMenuItemsCount() { return sizeof(toolsMenuItems) / sizeof(toolsMenuItems[0]); }
int getSettingsMenuItemsCount() { return sizeof(settingsMenuItems) / sizeof(settingsMenuItems[0]); }
int getUtilitiesMenuItemsCount() { return sizeof(utilitiesMenuItems) / sizeof(utilitiesMenuItems[0]); }
int getInjectionToolItemsCount() { return sizeof(injectionToolItems) / sizeof(injectionToolItems[0]); }
int getWifiAttackToolItemsCount() { return sizeof(wifiAttackToolItems) / sizeof(wifiAttackToolItems[0]); }
int getBleAttackToolItemsCount() { return sizeof(bleAttackToolItems) / sizeof(bleAttackToolItems[0]); }
int getNrfReconToolItemsCount() { return sizeof(nrfReconToolItems) / sizeof(nrfReconToolItems[0]); }
int getJammingToolItemsCount() { return sizeof(jammingToolItems) / sizeof(jammingToolItems[0]); }


// --- initializeCurrentMenu() --- (Complete and verified version from previous interactions)
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
        case UTILITIES_MENU: 
            maxMenuItems = getUtilitiesMenuItemsCount();
            subMenuAnim.init(); // Ensure subMenuAnim is initialized
            subMenuAnim.setTargets(menuIndex, maxMenuItems); // Set initial targets
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
            if (wifiIsScanning) { 
                maxMenuItems = 0; 
            } else {
                maxMenuItems = foundWifiNetworksCount + 2; 
            }
            wifiListAnim.init(); 
            if (maxMenuItems > 0) {
                if (wifiMenuIndex >= maxMenuItems) wifiMenuIndex = maxMenuItems - 1;
                if (wifiMenuIndex < 0) wifiMenuIndex = 0;
            } else {
                 wifiMenuIndex = 0;
            }
            wifiListAnim.setTargets(wifiMenuIndex, maxMenuItems); 
            break;
        case FLASHLIGHT_MODE: 
            maxMenuItems = 0; 
            break;
    }

    if (currentMenu != WIFI_SETUP_MENU) { 
        if (maxMenuItems > 0) {
            if (menuIndex >= maxMenuItems) menuIndex = maxMenuItems - 1;
            if (menuIndex < 0) menuIndex = 0;
        } else if (menuIndex != 0) { 
            menuIndex = 0;
        }
    }
}

// --- handleMenuSelection() --- (Focus of this fix)
void handleMenuSelection() {
    MenuState previousMenu = currentMenu; 
    int localMenuIndexForNewMenu = 0; // Default starting index for new submenus

    if (currentMenu == MAIN_MENU) {
        mainMenuSavedIndex = menuIndex; // Save the selection from the main menu
        MenuState targetMenu = currentMenu; // Default to no change if no match below

        // Ensure menuIndex is within bounds for mainMenuItems
        if (menuIndex < 0 || menuIndex >= getMainMenuItemsCount()) {
            // This should ideally not happen if navigation is correct
            Serial.println("Error: Main menu index out of bounds!");
            return; 
        }

        const char* selectedMainItem = mainMenuItems[menuIndex];

        if (strcmp(selectedMainItem, "Tools") == 0) {
            targetMenu = TOOLS_MENU;
        } else if (strcmp(selectedMainItem, "Games") == 0) {
            targetMenu = GAMES_MENU;
        } else if (strcmp(selectedMainItem, "Settings") == 0) {
            targetMenu = SETTINGS_MENU;
        } else if (strcmp(selectedMainItem, "Utilities") == 0) { // <--- CHECK THIS STRING MATCH
            targetMenu = UTILITIES_MENU;
        } else if (strcmp(selectedMainItem, "Info") == 0) {
            // Handle Info screen if it's a separate state, or just return if it's a static display
            Serial.println("Info selected (no state change implemented).");
            return; // No actual menu state change for "Info" in this example
        }
        
        // If targetMenu was changed by one of the conditions above
        if (targetMenu != currentMenu) { 
            currentMenu = targetMenu;
            menuIndex = localMenuIndexForNewMenu; // Reset to 0 for the new submenu
            initializeCurrentMenu();
        }
    } else if (currentMenu == TOOLS_MENU) {
        int currentMax = getToolsMenuItemsCount();
        if (menuIndex == (currentMax - 1)) { // "Back" selected
            currentMenu = MAIN_MENU; 
            // Restore selection to "Tools" in main menu (or use mainMenuSavedIndex if preferred)
            menuIndex = 0; // Default
            for(int i=0; i < getMainMenuItemsCount(); ++i) {
                if (strcmp(mainMenuItems[i], "Tools") == 0) {
                    menuIndex = i;
                    mainMenuSavedIndex = i;
                    break;
                }
            }
        } else { // A specific tool category selected
            toolsCategoryIndex = menuIndex; 
            currentMenu = TOOL_CATEGORY_GRID; 
            menuIndex = localMenuIndexForNewMenu; 
        }
        initializeCurrentMenu();
    } else if (currentMenu == GAMES_MENU) {
        int currentMax = getGamesMenuItemsCount();
        if (menuIndex == (currentMax - 1)) { // "Back"
            currentMenu = MAIN_MENU; 
            menuIndex = 0; // Default
            for(int i=0; i < getMainMenuItemsCount(); ++i) {
                if (strcmp(mainMenuItems[i], "Games") == 0) {
                    menuIndex = i;
                    mainMenuSavedIndex = i;
                    break;
                }
            }
        } else { 
            Serial.printf("Game selected: %s\n", gamesMenuItems[menuIndex]);
            // Game launch logic would go here (possibly changing currentMenu)
        }
        // Only initialize if menu actually changed (e.g. "Back" or game launch changed currentMenu)
        if (previousMenu != currentMenu) initializeCurrentMenu();

    } else if (currentMenu == SETTINGS_MENU) {
        int currentMax = getSettingsMenuItemsCount();
        if (menuIndex == (currentMax - 1)) { // "Back"
            currentMenu = MAIN_MENU; 
            menuIndex = 0; // Default
            for(int i=0; i < getMainMenuItemsCount(); ++i) {
                if (strcmp(mainMenuItems[i], "Settings") == 0) {
                    menuIndex = i;
                    mainMenuSavedIndex = i;
                    break;
                }
            }
        } else if (strcmp(settingsMenuItems[menuIndex], "Wi-Fi Setup") == 0) { 
            currentMenu = WIFI_SETUP_MENU;
            wifiMenuIndex = localMenuIndexForNewMenu; 
            // Initiate Wi-Fi scan
            int scanReturn = initiateAsyncWifiScan(); 
            if (scanReturn == WIFI_SCAN_RUNNING) { 
                wifiIsScanning = true; 
                foundWifiNetworksCount = 0; 
                lastWifiScanCheckTime = millis();
            } else if (scanReturn >= 0) { 
                wifiIsScanning = false; 
                checkAndRetrieveWifiScanResults(); 
            } else { 
                wifiIsScanning = false; 
                Serial.println("Failed to start Wi-Fi scan."); 
                foundWifiNetworksCount = 0;
            }
        } else { 
            Serial.printf("Setting selected: %s\n", settingsMenuItems[menuIndex]);
            // Actual setting change logic
        }
        if (previousMenu != currentMenu) initializeCurrentMenu();

    } else if (currentMenu == TOOL_CATEGORY_GRID) {
        int currentMax = 0;
        // Determine currentMax based on toolsCategoryIndex (as before)
        switch(toolsCategoryIndex) { 
            case 0: currentMax = getInjectionToolItemsCount(); break;
            case 1: currentMax = getWifiAttackToolItemsCount(); break;
            case 2: currentMax = getBleAttackToolItemsCount(); break;
            case 3: currentMax = getNrfReconToolItemsCount(); break;
            case 4: currentMax = getJammingToolItemsCount(); break;
            default: currentMax = 0; break;
        }

        if (menuIndex == currentMax - 1 && currentMax > 0) { // "Back" from tool grid
            currentMenu = TOOLS_MENU; 
            menuIndex = toolsCategoryIndex; // Select the category we came from in Tools Menu
        } else if (currentMax > 0) { 
             // Tool selected logic (e.g., print name)
             const char** currentToolList = nullptr;
             switch(toolsCategoryIndex){
                case 0: currentToolList = injectionToolItems; break;
                case 1: currentToolList = wifiAttackToolItems; break;
                case 2: currentToolList = bleAttackToolItems; break;
                case 3: currentToolList = nrfReconToolItems; break;
                case 4: currentToolList = jammingToolItems; break;
             }
             if (currentToolList && menuIndex < currentMax -1) { // Ensure not "Back"
                Serial.printf("Tool selected: %s\n", currentToolList[menuIndex]);
             }
        }
        // Always re-init if selection was made or backed out of grid
        // (as it might change currentMenu or require grid reset)
        initializeCurrentMenu(); 
    } else if (currentMenu == WIFI_SETUP_MENU) {
        if (!wifiIsScanning) { 
            if (wifiMenuIndex < foundWifiNetworksCount) { 
                Serial.print("Selected WiFi to connect: "); Serial.println(scannedNetworks[wifiMenuIndex].ssid);
                // TODO: Implement password entry / connection logic
            } else if (wifiMenuIndex == foundWifiNetworksCount) { // "Scan Again"
                Serial.println("Re-initiating Wi-Fi scan...");
                int scanReturn = initiateAsyncWifiScan();
                if (scanReturn == WIFI_SCAN_RUNNING) {
                    wifiIsScanning = true; 
                    foundWifiNetworksCount = 0; 
                    // wifiMenuIndex will be reset by initializeCurrentMenu or loop() logic
                    lastWifiScanCheckTime = millis();
                } else if (scanReturn >= 0) {
                    wifiIsScanning = false; 
                    checkAndRetrieveWifiScanResults();
                } else {
                    wifiIsScanning = false; Serial.println("Failed to start re-scan.");
                }
                initializeCurrentMenu(); 
                wifiMenuIndex = 0; 
                targetWifiListScrollOffset_Y = 0; 
                currentWifiListScrollOffset_Y_anim = 0;
                if(currentMenu == WIFI_SETUP_MENU) wifiListAnim.setTargets(wifiMenuIndex, maxMenuItems);

            } else if (wifiMenuIndex == foundWifiNetworksCount + 1) { // "Back" from Wi-Fi
                currentMenu = SETTINGS_MENU;
                menuIndex = 0; // Default
                for(int i=0; i < getSettingsMenuItemsCount(); ++i) {
                    if (strcmp(settingsMenuItems[i], "Wi-Fi Setup") == 0) {
                        menuIndex = i; 
                        break;
                    }
                }
                initializeCurrentMenu();
            }
        }
    } else if (currentMenu == UTILITIES_MENU) { 
        const char* selectedItem = utilitiesMenuItems[menuIndex];
        bool menuChanged = false; // Flag to track if currentMenu changes

        if (strcmp(selectedItem, "Vibration") == 0) {
            vibrationOn = !vibrationOn;
            setOutputOnPCF0(MOTOR_PIN_PCF0, vibrationOn);
            // No menu change, but carousel redraw will show updated text
        } else if (strcmp(selectedItem, "Laser") == 0) {
            laserOn = !laserOn;
            setOutputOnPCF0(LASER_PIN_PCF0, laserOn);
            // No menu change
        } else if (strcmp(selectedItem, "Flashlight") == 0) {
            currentMenu = FLASHLIGHT_MODE;
            menuChanged = true; 
        } else if (strcmp(selectedItem, "Back") == 0) {
            currentMenu = MAIN_MENU;
            menuIndex = 0; // Default
            for(int i=0; i < getMainMenuItemsCount(); ++i) { 
                if (strcmp(mainMenuItems[i], "Utilities") == 0) {
                    menuIndex = i;
                    mainMenuSavedIndex = i; 
                    break;
                }
            }
            menuChanged = true;
        }
        if (menuChanged) initializeCurrentMenu(); // Initialize only if menu actually changed
                                                // Otherwise, carousel handles its own visual updates for toggles

    } else if (currentMenu == FLASHLIGHT_MODE) { 
        currentMenu = UTILITIES_MENU; 
        menuIndex = 0; 
        for(int i=0; i < getUtilitiesMenuItemsCount(); ++i) {
            if (strcmp(utilitiesMenuItems[i], "Flashlight") == 0) {
                menuIndex = i;
                break;
            }
        }
        initializeCurrentMenu();
    }
}

// --- handleMenuBackNavigation() --- (Complete and verified version from previous interactions)
void handleMenuBackNavigation() {
    MenuState previousMenuState = currentMenu; 

    if (currentMenu == MAIN_MENU) {
        return; 
    } else if (currentMenu == TOOL_CATEGORY_GRID) {
        currentMenu = TOOLS_MENU;
        menuIndex = toolsCategoryIndex; 
    } else if (currentMenu == WIFI_SETUP_MENU) { 
        if (wifiIsScanning) { 
            wifiIsScanning = false; 
        }
        currentMenu = SETTINGS_MENU;
        menuIndex = 0; 
        for(int i=0; i < getSettingsMenuItemsCount(); ++i) { 
            if (strcmp(settingsMenuItems[i], "Wi-Fi Setup") == 0) {
                menuIndex = i;
                break;
            }
        }
    } else if (currentMenu == UTILITIES_MENU) { 
        currentMenu = MAIN_MENU;
        menuIndex = 0; 
        for(int i=0; i<getMainMenuItemsCount(); ++i) {
            if(strcmp(mainMenuItems[i], "Utilities") == 0) {
                menuIndex = i;
                break;
            }
        }
        mainMenuSavedIndex = menuIndex; 
    } else if (currentMenu == FLASHLIGHT_MODE) { 
        currentMenu = UTILITIES_MENU;
        menuIndex = 0; 
        for(int i=0; i < getUtilitiesMenuItemsCount(); ++i) {
            if (strcmp(utilitiesMenuItems[i], "Flashlight") == 0) {
                menuIndex = i;
                break;
            }
        }
    }
    else if (currentMenu == GAMES_MENU || currentMenu == TOOLS_MENU || currentMenu == SETTINGS_MENU) {
        // This case needs to be before UTILITIES_MENU if UTILITIES_MENU is a peer
        // If navigating back from Games, Tools, Settings directly to MainMenu
        currentMenu = MAIN_MENU;
        // Find the mainMenuSavedIndex based on the previousMenuState
        if (previousMenuState == GAMES_MENU) {
            for(int i=0; i<getMainMenuItemsCount(); ++i) if(strcmp(mainMenuItems[i], "Games") == 0) { mainMenuSavedIndex = i; break; }
        } else if (previousMenuState == TOOLS_MENU) {
            for(int i=0; i<getMainMenuItemsCount(); ++i) if(strcmp(mainMenuItems[i], "Tools") == 0) { mainMenuSavedIndex = i; break; }
        } else if (previousMenuState == SETTINGS_MENU) {
            for(int i=0; i<getMainMenuItemsCount(); ++i) if(strcmp(mainMenuItems[i], "Settings") == 0) { mainMenuSavedIndex = i; break; }
        }
        menuIndex = mainMenuSavedIndex; 
    } 
    
    if (previousMenuState != currentMenu) {
        initializeCurrentMenu(); 
    }
}