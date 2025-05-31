#include "menu_logic.h"
// No need to include ui_drawing.h again if config.h includes it, or if animation objects are passed.
// However, for direct access to mainMenuAnim and subMenuAnim, they need to be visible.
// This is handled by them being extern in config.h and defined in KivaMain.ino.

// --- Menu Item Definitions ---
const char* mainMenuItems[] = { "Tools", "Games", "Settings", "Info" };
const char* gamesMenuItems[] = { "Snake Game", "Tetris Clone", "Classic Pong", "2D Maze", "Back" };
const char* toolsMenuItems[] = { "Injection", "Wi-Fi Attacks", "BLE/BT Attacks", "NRF Recon", "Jamming", "Back" };
const char* injectionToolItems[] = { "MouseJack", "Wireless Keystroke", "Fake HID", "BLE HID Inject", "RF Payload", "Back" };
const char* wifiAttackToolItems[] = { "Beacon Spam", "Deauth Attack", "Probe Flood", "Fake AP", "Karma AP", "DNS Spoof", "Packet Inject", "Packet Replay", "Back" };
const char* bleAttackToolItems[] = { "BLE Ad Spam", "BLE Scanner", "BLE DoS", "BLE Name Spoof", "BT MAC Spoof", "Back" };
const char* nrfReconToolItems[] = { "NRF Scanner", "ShockBurst Sniff", "Key Sniffer", "NRF Brute", "Custom Flood", "RF Spammer", "BLE Ad Disrupt", "Dual Interference", "Back" };
const char* jammingToolItems[] = { "NRF Jam", "Wi-Fi Deauth Jam", "BLE Flood Jam", "Channel Hop Jam", "RF Noise Flood", "Back" };
const char* settingsMenuItems[] = { "Display Options", "Sound Setup", "System Info", "About Kiva", "Back" };


void initializeCurrentMenu() {
    marqueeActive = false; 
    marqueeOffset = 0;
    marqueeScrollLeft = true;
    gridAnimatingIn = false; 

    switch (currentMenu) {
        case MAIN_MENU:
            maxMenuItems = MAIN_MENU_ITEMS_COUNT; // <--- USE COUNT
            mainMenuAnim.init(); 
            mainMenuAnim.setTargets(menuIndex, maxMenuItems);
            break;
        case GAMES_MENU:
            maxMenuItems = GAMES_MENU_ITEMS_COUNT; // <--- USE COUNT
            subMenuAnim.init();
            subMenuAnim.setTargets(menuIndex, maxMenuItems);
            break;
        case TOOLS_MENU:
            maxMenuItems = TOOLS_MENU_ITEMS_COUNT; // <--- USE COUNT
            subMenuAnim.init();
            subMenuAnim.setTargets(menuIndex, maxMenuItems);
            break;
        case SETTINGS_MENU:
            maxMenuItems = SETTINGS_MENU_ITEMS_COUNT; // <--- USE COUNT
            subMenuAnim.init();
            subMenuAnim.setTargets(menuIndex, maxMenuItems);
            break;
        case TOOL_CATEGORY_GRID:
            switch (toolsCategoryIndex) {
                case 0: maxMenuItems = INJECTION_TOOL_ITEMS_COUNT; break; // <--- USE COUNT
                case 1: maxMenuItems = WIFI_ATTACK_TOOL_ITEMS_COUNT; break; // <--- USE COUNT
                case 2: maxMenuItems = BLE_ATTACK_TOOL_ITEMS_COUNT; break; // <--- USE COUNT
                case 3: maxMenuItems = NRF_RECON_TOOL_ITEMS_COUNT; break; // <--- USE COUNT
                case 4: maxMenuItems = JAMMING_TOOL_ITEMS_COUNT; break; // <--- USE COUNT
                default: maxMenuItems = 0; break;
            }
            targetGridScrollOffset_Y = 0; 
            currentGridScrollOffset_Y_anim = 0;
            startGridItemAnimation(); 
            break;
    }
}


void handleMenuSelection() {
    MenuState previousMenu = currentMenu; 

    if (currentMenu == MAIN_MENU) {
        mainMenuSavedIndex = menuIndex; 
        if (menuIndex == 0) { currentMenu = TOOLS_MENU; }
        else if (menuIndex == 1) { currentMenu = GAMES_MENU; }
        else if (menuIndex == 2) { currentMenu = SETTINGS_MENU; }
        else if (menuIndex == 3) { /* Info selected */ }
        
        if (previousMenu != currentMenu) { 
            menuIndex = 0; 
            initializeCurrentMenu(); // This will set the correct maxMenuItems using _COUNT
        }
    } else if (currentMenu == TOOLS_MENU) {
        if (menuIndex == (TOOLS_MENU_ITEMS_COUNT - 1)) { // "Back" selected // <--- USE COUNT
            currentMenu = MAIN_MENU;
            menuIndex = mainMenuSavedIndex; 
        } else { 
            toolsCategoryIndex = menuIndex;
            currentMenu = TOOL_CATEGORY_GRID;
            menuIndex = 0; 
        }
        initializeCurrentMenu();
    } else if (currentMenu == GAMES_MENU) {
        if (menuIndex == (GAMES_MENU_ITEMS_COUNT - 1)) { // "Back" // <--- USE COUNT
            currentMenu = MAIN_MENU;
            menuIndex = mainMenuSavedIndex;
        } else { /* A game selected */ }
        initializeCurrentMenu();
    } else if (currentMenu == SETTINGS_MENU) {
        if (menuIndex == (SETTINGS_MENU_ITEMS_COUNT - 1)) { // "Back" // <--- USE COUNT
            currentMenu = MAIN_MENU;
            menuIndex = mainMenuSavedIndex;
        } else { /* A setting selected */ }
        initializeCurrentMenu();
    } else if (currentMenu == TOOL_CATEGORY_GRID) {
        int currentToolListSize = 0;
        // Determine which tool list we are in to get its size
        switch(toolsCategoryIndex) {
            case 0: currentToolListSize = INJECTION_TOOL_ITEMS_COUNT; break; // <--- USE COUNT
            case 1: currentToolListSize = WIFI_ATTACK_TOOL_ITEMS_COUNT; break; // <--- USE COUNT
            case 2: currentToolListSize = BLE_ATTACK_TOOL_ITEMS_COUNT; break; // <--- USE COUNT
            case 3: currentToolListSize = NRF_RECON_TOOL_ITEMS_COUNT; break; // <--- USE COUNT
            case 4: currentToolListSize = JAMMING_TOOL_ITEMS_COUNT; break; // <--- USE COUNT
            default: break;
        }

        if (menuIndex == currentToolListSize - 1 && currentToolListSize > 0) { // "Back" from grid
            currentMenu = TOOLS_MENU;
            menuIndex = toolsCategoryIndex; 
        } else if (currentToolListSize > 0) { /* Actual tool selected */ }
        initializeCurrentMenu();
    }
}

void handleMenuBackNavigation() {
    if (currentMenu == TOOL_CATEGORY_GRID) {
        currentMenu = TOOLS_MENU;
        menuIndex = toolsCategoryIndex; // Pre-select the category we came from
    } else if (currentMenu == GAMES_MENU || currentMenu == TOOLS_MENU || currentMenu == SETTINGS_MENU) {
        currentMenu = MAIN_MENU;
        menuIndex = mainMenuSavedIndex; // Restore where we were in main menu
    } else {
        // In MAIN_MENU or a non-navigable screen, NAV_BACK might do nothing or exit an app
    }
    initializeCurrentMenu(); // Re-initialize animations and maxItems for the new (or old) menu
}