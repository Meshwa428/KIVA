// KivaMain/menu_logic.cpp

#include "menu_logic.h"
#include "config.h"
#include "ui_drawing.h"
#include "wifi_manager.h"
#include "pcf_utils.h"
#include "keyboard_layout.h"
#include "jamming.h"

// --- Menu Item Arrays ---
const char* mainMenuItems[] = {
  "Tools", "Games", "Settings", "Utilities", "Info"
};
const char* gamesMenuItems[] = { "Snake Game", "Tetris Clone", "Classic Pong", "2D Maze", "Back" };
const char* settingsMenuItems[] = { "Wi-Fi Setup", "Display Opts", "Sound Setup", "System Info", "About Kiva", "Back" };
const char* utilitiesMenuItems[] = { "Vibration", "Laser", "Flashlight", "Back" };
const char* toolsMenuItems[] = { "Injection", "Wi-Fi Attacks", "BLE/BT Attacks", "NRF Recon", "Jamming", "Back" }; // Jamming is index 4
const char* injectionToolItems[] = { "MouseJack", "Wireless Keystroke", "Fake HID", "BLE HID Inject", "RF Payload", "Back" };
const char* wifiAttackToolItems[] = { "Beacon Spam", "Deauth Attack", "Probe Flood", "Fake AP", "Karma AP", "DNS Spoof", "Packet Inject", "Packet Replay", "Back" };
const char* bleAttackToolItems[] = { "BLE Ad Spam", "BLE Scanner", "BLE DoS", "BLE Name Spoof", "BT MAC Spoof", "Back" };
const char* nrfReconToolItems[] = { "NRF Scanner", "ShockBurst Sniff", "Key Sniffer", "NRF Brute", "Custom Flood", "RF Spammer", "BLE Ad Disrupt", "Dual Interference", "Back" };
const char* jammingToolItems[] = {
  "BLE Jam",         // Index 0
  "BT Jam",          // Index 1
  "NRF Channel Jam", // Index 2
  "RF Noise Flood",  // Index 3
  "Back"             // Index 4
};

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


void initializeCurrentMenu() {
  marqueeActive = false;
  marqueeOffset = 0;
  marqueeScrollLeft = true;

  if (currentMenu != WIFI_PASSWORD_INPUT) {
    keyboardFocusRow = 0;
    keyboardFocusCol = 0;
  }

  if (!isJammingOperationActive) {
      currentBatteryCheckInterval = BATTERY_CHECK_INTERVAL_NORMAL;
      currentInputPollInterval = INPUT_POLL_INTERVAL_NORMAL;
  }

  // Determine maxMenuItems first based on the currentMenu and its specific context (like toolsCategoryIndex)
  switch (currentMenu) {
    case MAIN_MENU: maxMenuItems = getMainMenuItemsCount(); break;
    case GAMES_MENU: maxMenuItems = getGamesMenuItemsCount(); break;
    case TOOLS_MENU: maxMenuItems = getToolsMenuItemsCount(); break;
    case SETTINGS_MENU: maxMenuItems = getSettingsMenuItemsCount(); break;
    case UTILITIES_MENU: maxMenuItems = getUtilitiesMenuItemsCount(); break;
    case TOOL_CATEGORY_GRID:
      // Assuming "Jamming" is toolsMenuItems[4]
      if (toolsCategoryIndex == 4 && strcmp(toolsMenuItems[toolsCategoryIndex], "Jamming") == 0) {
          maxMenuItems = getJammingToolItemsCount();
      } else {
        switch (toolsCategoryIndex) {
          case 0: maxMenuItems = getInjectionToolItemsCount(); break;
          case 1: maxMenuItems = getWifiAttackToolItemsCount(); break;
          case 2: maxMenuItems = getBleAttackToolItemsCount(); break;
          case 3: maxMenuItems = getNrfReconToolItemsCount(); break;
          default: maxMenuItems = 0; break;
        }
      }
      break;
    case WIFI_SETUP_MENU:
      if (!wifiHardwareEnabled) { maxMenuItems = 2; }
      else if (wifiIsScanning) { maxMenuItems = 0; }
      else { maxMenuItems = foundWifiNetworksCount + 2; }
      break;
    case FLASHLIGHT_MODE:
    case WIFI_PASSWORD_INPUT:
    case WIFI_CONNECTING:
    case WIFI_CONNECTION_INFO:
    case JAMMING_ACTIVE_SCREEN:
      maxMenuItems = 0;
      break;
    default: maxMenuItems = 0; break;
  }

  // Validate menuIndex against the determined maxMenuItems
  // This is crucial: menuIndex might have been set by handleMenuBackNavigation or handleMenuSelection
  if (maxMenuItems > 0) {
    if (menuIndex >= maxMenuItems) menuIndex = maxMenuItems - 1;
    if (menuIndex < 0) menuIndex = 0;
  } else { // If no items, index must be 0
    menuIndex = 0;
  }

  // Now, specific initializations for each menu state
  switch (currentMenu) {
    case MAIN_MENU:
      mainMenuAnim.startIntro(menuIndex, maxMenuItems);
      gridAnimatingIn = false;
      break;
    case GAMES_MENU:
    case TOOLS_MENU:
    case SETTINGS_MENU:
    case UTILITIES_MENU:
      subMenuAnim.init();
      subMenuAnim.setTargets(menuIndex, maxMenuItems);
      gridAnimatingIn = false;
      break;
    case TOOL_CATEGORY_GRID:
      targetGridScrollOffset_Y = 0;
      currentGridScrollOffset_Y_anim = 0;
      startGridItemAnimation(); // Relies on the now-validated menuIndex
      break;
    case WIFI_SETUP_MENU:
      wifiListAnim.init();
      // wifiMenuIndex is used for this menu, ensure it's also valid
      if (maxMenuItems > 0) {
        if (wifiMenuIndex >= maxMenuItems) wifiMenuIndex = maxMenuItems - 1;
        else if (wifiMenuIndex < 0) wifiMenuIndex = 0;
      } else {
        wifiMenuIndex = 0;
      }
      wifiListAnim.setTargets(wifiMenuIndex, maxMenuItems);
      targetWifiListScrollOffset_Y = 0;
      currentWifiListScrollOffset_Y_anim = 0;
      gridAnimatingIn = false;
      break;
    case FLASHLIGHT_MODE: // No specific animation, just ensure gridAnimatingIn is false
    case WIFI_PASSWORD_INPUT:
    case WIFI_CONNECTING:
    case WIFI_CONNECTION_INFO:
      gridAnimatingIn = false;
      break;
    case JAMMING_ACTIVE_SCREEN:
      gridAnimatingIn = false;
      currentBatteryCheckInterval = BATTERY_CHECK_INTERVAL_JAMMING;
      currentInputPollInterval = INPUT_POLL_INTERVAL_JAMMING;
      break;
  }
}

void handleMenuSelection() {
  MenuState previousMenu = currentMenu;
  int localMenuIndexForNewMenu = 0;

  if (currentMenu == MAIN_MENU) {
    mainMenuSavedIndex = menuIndex;
    MenuState targetMenu = currentMenu;
    if (menuIndex < 0 || menuIndex >= getMainMenuItemsCount()) {
      Serial.println("Error: Main menu index out of bounds!");
      return;
    }
    const char* selectedMainItem = mainMenuItems[menuIndex];
    if (strcmp(selectedMainItem, "Tools") == 0) targetMenu = TOOLS_MENU;
    else if (strcmp(selectedMainItem, "Games") == 0) targetMenu = GAMES_MENU;
    else if (strcmp(selectedMainItem, "Settings") == 0) targetMenu = SETTINGS_MENU;
    else if (strcmp(selectedMainItem, "Utilities") == 0) targetMenu = UTILITIES_MENU;
    else if (strcmp(selectedMainItem, "Info") == 0) {
      Serial.println("Info selected - (Not yet implemented)");
      return;
    }

    if (targetMenu != currentMenu) {
      currentMenu = targetMenu;
      menuIndex = localMenuIndexForNewMenu;
      initializeCurrentMenu();
    }
  } else if (currentMenu == TOOLS_MENU) {
    int currentMax = getToolsMenuItemsCount();
    if (menuIndex == (currentMax - 1)) { // "Back"
      currentMenu = MAIN_MENU;
      menuIndex = mainMenuSavedIndex; // Restore selection in main menu
    } else { // A tool category selected
      toolsCategoryIndex = menuIndex;
      currentMenu = TOOL_CATEGORY_GRID;
      menuIndex = localMenuIndexForNewMenu; // Reset index for the new grid
    }
    initializeCurrentMenu();
  } else if (currentMenu == GAMES_MENU) {
    int currentMax = getGamesMenuItemsCount();
    if (menuIndex == (currentMax - 1)) { // "Back"
      currentMenu = MAIN_MENU;
      menuIndex = mainMenuSavedIndex;
      initializeCurrentMenu();
    } else {
      Serial.printf("Selected game: %s (Not yet implemented)\n", gamesMenuItems[menuIndex]);
    }
  } else if (currentMenu == SETTINGS_MENU) {
    int currentMax = getSettingsMenuItemsCount();
    if (menuIndex == (currentMax - 1)) { // "Back"
      currentMenu = MAIN_MENU;
      menuIndex = mainMenuSavedIndex;
      initializeCurrentMenu();
    } else if (strcmp(settingsMenuItems[menuIndex], "Wi-Fi Setup") == 0) {
      setWifiHardwareState(true);
      delay(100);
      currentMenu = WIFI_SETUP_MENU;
      wifiMenuIndex = localMenuIndexForNewMenu;
      int scanReturn = initiateAsyncWifiScan();
      if (scanReturn == WIFI_SCAN_RUNNING || scanReturn == -1 ) {
        wifiIsScanning = true; foundWifiNetworksCount = 0; lastWifiScanCheckTime = millis();
      } else if (scanReturn == -3 ) {
        wifiIsScanning = false; foundWifiNetworksCount = 0;
      } else {
        wifiIsScanning = false; checkAndRetrieveWifiScanResults();
      }
      initializeCurrentMenu();
    } else {
      Serial.printf("Selected setting: %s (Not yet implemented)\n", settingsMenuItems[menuIndex]);
    }
  } else if (currentMenu == TOOL_CATEGORY_GRID) {
    int currentMaxItemsInGrid = 0;
    bool isJammingCategory = false;
    // "Jamming" is toolsMenuItems[4]
    if (toolsCategoryIndex == 4 && strcmp(toolsMenuItems[toolsCategoryIndex], "Jamming") == 0) {
        isJammingCategory = true;
        currentMaxItemsInGrid = getJammingToolItemsCount();
    } else {
       switch (toolsCategoryIndex) {
        case 0: currentMaxItemsInGrid = getInjectionToolItemsCount(); break;
        case 1: currentMaxItemsInGrid = getWifiAttackToolItemsCount(); break;
        case 2: currentMaxItemsInGrid = getBleAttackToolItemsCount(); break;
        case 3: currentMaxItemsInGrid = getNrfReconToolItemsCount(); break;
        default: currentMaxItemsInGrid = 0; break;
      }
    }

    if (menuIndex == currentMaxItemsInGrid - 1 && currentMaxItemsInGrid > 0) { // "Back" selected
      currentMenu = TOOLS_MENU;
      menuIndex = toolsCategoryIndex; // Select the category in the parent Tools menu
      initializeCurrentMenu();
    } else if (isJammingCategory && menuIndex < (currentMaxItemsInGrid - 1) ) { // A jamming mode selected
        lastSelectedJammingToolGridIndex = menuIndex; // STORE THE CURRENTLY SELECTED JAMMING TOOL'S INDEX
        JammingType jamTypeToStart = JAM_NONE;
        const char* selectedJamItem = jammingToolItems[menuIndex];

        if (strcmp(selectedJamItem, "BLE Jam") == 0) jamTypeToStart = JAM_BLE;
        else if (strcmp(selectedJamItem, "BT Jam") == 0) jamTypeToStart = JAM_BT;
        else if (strcmp(selectedJamItem, "NRF Channel Jam") == 0) jamTypeToStart = JAM_NRF_CHANNELS;
        else if (strcmp(selectedJamItem, "RF Noise Flood") == 0) jamTypeToStart = JAM_RF_NOISE_FLOOD;

        if (jamTypeToStart != JAM_NONE) {
            if (startActiveJamming(jamTypeToStart)) {
                currentMenu = JAMMING_ACTIVE_SCREEN;
                initializeCurrentMenu();
            } else {
                Serial.println("Menu: Failed to start jamming operation.");
            }
        }
    } else if (!isJammingCategory && currentMaxItemsInGrid > 0 && menuIndex < (currentMaxItemsInGrid -1)) {
      const char* catName = toolsMenuItems[toolsCategoryIndex];
      const char* itemName = "Unknown";
      if (toolsCategoryIndex == 0 && menuIndex < getInjectionToolItemsCount()) itemName = injectionToolItems[menuIndex];
      else if (toolsCategoryIndex == 1 && menuIndex < getWifiAttackToolItemsCount()) itemName = wifiAttackToolItems[menuIndex];
      else if (toolsCategoryIndex == 2 && menuIndex < getBleAttackToolItemsCount()) itemName = bleAttackToolItems[menuIndex];
      else if (toolsCategoryIndex == 3 && menuIndex < getNrfReconToolItemsCount()) itemName = nrfReconToolItems[menuIndex];
      Serial.printf("Tool '%s' from category '%s' selected (Not yet implemented)\n", itemName, catName);
    } else {
      initializeCurrentMenu(); // Fallback
    }
  } else if (currentMenu == WIFI_SETUP_MENU) {
    if (!wifiHardwareEnabled) {
        if (maxMenuItems > 0 && wifiMenuIndex == (maxMenuItems -1) ) { // "Back"
             currentMenu = SETTINGS_MENU;
             for (int i = 0; i < getSettingsMenuItemsCount(); ++i) if (strcmp(settingsMenuItems[i], "Wi-Fi Setup") == 0) { menuIndex = i; break; }
             initializeCurrentMenu();
        } else if (maxMenuItems > 0 && wifiMenuIndex == 0) { // "Enable Wi-Fi"
            setWifiHardwareState(true); delay(100);
            int scanReturn = initiateAsyncWifiScan();
            if (scanReturn == WIFI_SCAN_RUNNING || scanReturn == -1) { wifiIsScanning = true; foundWifiNetworksCount = 0; lastWifiScanCheckTime = millis(); }
            else { wifiIsScanning = false; checkAndRetrieveWifiScanResults(); }
            initializeCurrentMenu(); wifiMenuIndex = 0;
            if(maxMenuItems > 0) wifiListAnim.setTargets(wifiMenuIndex, maxMenuItems);
        }
        return;
    }
    if (!wifiIsScanning) {
      if (wifiMenuIndex < foundWifiNetworksCount) {
        if (currentConnectedSsid.length() > 0 && strcmp(scannedNetworks[wifiMenuIndex].ssid, currentConnectedSsid.c_str()) == 0) {
            strncpy(currentSsidToConnect, scannedNetworks[wifiMenuIndex].ssid, sizeof(currentSsidToConnect) - 1); currentSsidToConnect[sizeof(currentSsidToConnect) - 1] = '\0';
            showWifiDisconnectOverlay = true; disconnectOverlaySelection = 0; disconnectOverlayAnimatingIn = true;
            disconnectOverlayCurrentScale = 0.1f; disconnectOverlayTargetScale = 1.0f; disconnectOverlayAnimStartTime = millis();
            return;
        } else {
            strncpy(currentSsidToConnect, scannedNetworks[wifiMenuIndex].ssid, sizeof(currentSsidToConnect) - 1); currentSsidToConnect[sizeof(currentSsidToConnect) - 1] = '\0';
            selectedNetworkIsSecure = scannedNetworks[wifiMenuIndex].isSecure;
            KnownWifiNetwork* knownNet = findKnownNetwork(currentSsidToConnect);
            if (selectedNetworkIsSecure) {
                if (knownNet && knownNet->failCount < MAX_WIFI_FAIL_ATTEMPTS && strlen(knownNet->password) > 0) {
                    attemptWpaWifiConnection(currentSsidToConnect, knownNet->password); currentMenu = WIFI_CONNECTING;
                } else {
                    currentMenu = WIFI_PASSWORD_INPUT; wifiPasswordInput[0] = '\0'; wifiPasswordInputCursor = 0;
                }
            } else {
                attemptDirectWifiConnection(currentSsidToConnect); currentMenu = WIFI_CONNECTING;
            }
            initializeCurrentMenu();
        }
      } else if (wifiMenuIndex == foundWifiNetworksCount) {  // "Scan Again"
        int scanReturn = initiateAsyncWifiScan();
        if (scanReturn == WIFI_SCAN_RUNNING || scanReturn == -1) { wifiIsScanning = true; foundWifiNetworksCount = 0; lastWifiScanCheckTime = millis(); }
        else { wifiIsScanning = false; checkAndRetrieveWifiScanResults(); }
        initializeCurrentMenu(); wifiMenuIndex = 0; targetWifiListScrollOffset_Y = 0; currentWifiListScrollOffset_Y_anim = 0;
        if (currentMenu == WIFI_SETUP_MENU && maxMenuItems > 0) wifiListAnim.setTargets(wifiMenuIndex, maxMenuItems);
      } else if (wifiMenuIndex == foundWifiNetworksCount + 1) {  // "Back"
        currentMenu = SETTINGS_MENU;
        for (int i = 0; i < getSettingsMenuItemsCount(); ++i) if (strcmp(settingsMenuItems[i], "Wi-Fi Setup") == 0) { menuIndex = i; break; }
        initializeCurrentMenu();
      }
    }
  } else if (currentMenu == UTILITIES_MENU) {
    const char* selectedItem = utilitiesMenuItems[menuIndex];
    bool menuChanged = false;
    if (strcmp(selectedItem, "Vibration") == 0) { vibrationOn = !vibrationOn; setOutputOnPCF0(MOTOR_PIN_PCF0, vibrationOn); }
    else if (strcmp(selectedItem, "Laser") == 0) { laserOn = !laserOn; setOutputOnPCF0(LASER_PIN_PCF0, laserOn); }
    else if (strcmp(selectedItem, "Flashlight") == 0) { currentMenu = FLASHLIGHT_MODE; menuChanged = true; }
    else if (strcmp(selectedItem, "Back") == 0) {
      currentMenu = MAIN_MENU; menuChanged = true;
      for (int i = 0; i < getMainMenuItemsCount(); ++i) if (strcmp(mainMenuItems[i], "Utilities") == 0) { menuIndex = i; mainMenuSavedIndex = i; break; }
    }
    if (menuChanged) initializeCurrentMenu();
  } else if (currentMenu == FLASHLIGHT_MODE) {
    currentMenu = UTILITIES_MENU;
    for (int i = 0; i < getUtilitiesMenuItemsCount(); ++i) if (strcmp(utilitiesMenuItems[i], "Flashlight") == 0) { menuIndex = i; break; }
    initializeCurrentMenu();
  }
}

void handleMenuBackNavigation() {
  MenuState previousMenuState = currentMenu;

  if (currentMenu == JAMMING_ACTIVE_SCREEN) {
    stopActiveJamming();
    currentMenu = TOOL_CATEGORY_GRID;

    // Explicitly set toolsCategoryIndex to "Jamming" (assuming index 4)
    // This is crucial if toolsCategoryIndex might have been changed elsewhere, though unlikely from JAMMING_ACTIVE_SCREEN
    bool foundJammingCat = false;
    for(int i=0; i < getToolsMenuItemsCount(); ++i) {
        if(strcmp(toolsMenuItems[i], "Jamming") == 0) {
            toolsCategoryIndex = i;
            foundJammingCat = true;
            break;
        }
    }
    if (!foundJammingCat) { // Fallback, should not happen if toolsMenuItems is consistent
        toolsCategoryIndex = 4; 
    }
    
    menuIndex = lastSelectedJammingToolGridIndex; // Restore the specific jamming tool

    // Bounds check for menuIndex within the context of jammingToolItems
    // Max items for jamming grid will be set in initializeCurrentMenu
    int numJammingItems = getJammingToolItemsCount();
    if (menuIndex < 0 || menuIndex >= numJammingItems) {
        menuIndex = numJammingItems > 0 ? (numJammingItems - 1) : 0; // Default to "Back" or 0
    }
    // initializeCurrentMenu will handle setting maxMenuItems and further validation for this new state.

  } else if (currentMenu == MAIN_MENU) {
    return;
  } else if (currentMenu == WIFI_PASSWORD_INPUT) {
    currentMenu = WIFI_SETUP_MENU;
    wifiPasswordInput[0] = '\0'; wifiPasswordInputCursor = 0;
  } else if (currentMenu == WIFI_CONNECTING || currentMenu == WIFI_CONNECTION_INFO) {
    if (WiFi.status() != WL_NO_SHIELD && WiFi.status() != WL_IDLE_STATUS ) { WiFi.disconnect(true); delay(100); }
    currentMenu = WIFI_SETUP_MENU; wifiMenuIndex = 0; targetWifiListScrollOffset_Y = 0; currentWifiListScrollOffset_Y_anim = 0;
  } else if (currentMenu == TOOL_CATEGORY_GRID) {
    currentMenu = TOOLS_MENU;
    menuIndex = toolsCategoryIndex; // Restore selection to the category name in Tools menu
    if (isJammingOperationActive) { stopActiveJamming(); }
  } else if (currentMenu == WIFI_SETUP_MENU) {
    if (wifiIsScanning) { wifiIsScanning = false; }
    currentMenu = SETTINGS_MENU;
    for (int i = 0; i < getSettingsMenuItemsCount(); ++i) if (strcmp(settingsMenuItems[i], "Wi-Fi Setup") == 0) { menuIndex = i; break; }
    if (WiFi.status() != WL_CONNECTED && wifiHardwareEnabled) { setWifiHardwareState(false); }
  } else if (currentMenu == UTILITIES_MENU) {
    currentMenu = MAIN_MENU;
    for (int i = 0; i < getMainMenuItemsCount(); ++i) if (strcmp(mainMenuItems[i], "Utilities") == 0) { menuIndex = i; mainMenuSavedIndex = i; break; }
  } else if (currentMenu == FLASHLIGHT_MODE) {
    currentMenu = UTILITIES_MENU;
    for (int i = 0; i < getUtilitiesMenuItemsCount(); ++i) if (strcmp(utilitiesMenuItems[i], "Flashlight") == 0) { menuIndex = i; break; }
  } else if (currentMenu == GAMES_MENU || currentMenu == TOOLS_MENU || currentMenu == SETTINGS_MENU) {
    currentMenu = MAIN_MENU;
    if (previousMenuState == GAMES_MENU) { for (int i = 0; i < getMainMenuItemsCount(); ++i) if (strcmp(mainMenuItems[i], "Games") == 0) { mainMenuSavedIndex = i; break; } }
    else if (previousMenuState == TOOLS_MENU) { for (int i = 0; i < getMainMenuItemsCount(); ++i) if (strcmp(mainMenuItems[i], "Tools") == 0) { mainMenuSavedIndex = i; break; } }
    else if (previousMenuState == SETTINGS_MENU) { for (int i = 0; i < getMainMenuItemsCount(); ++i) if (strcmp(mainMenuItems[i], "Settings") == 0) { mainMenuSavedIndex = i; break; } }
    menuIndex = mainMenuSavedIndex;
  }

  if (previousMenuState != currentMenu) {
    initializeCurrentMenu();
  }
}