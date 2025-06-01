#include "menu_logic.h"
#include "config.h"
#include "ui_drawing.h"
#include "wifi_manager.h"  // For connection attempts
#include "pcf_utils.h"
#include "keyboard_layout.h"  // For keyboard layer enums

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
int getMainMenuItemsCount() {
  return sizeof(mainMenuItems) / sizeof(mainMenuItems[0]);
}
int getGamesMenuItemsCount() {
  return sizeof(gamesMenuItems) / sizeof(gamesMenuItems[0]);
}
int getToolsMenuItemsCount() {
  return sizeof(toolsMenuItems) / sizeof(toolsMenuItems[0]);
}
int getSettingsMenuItemsCount() {
  return sizeof(settingsMenuItems) / sizeof(settingsMenuItems[0]);
}
int getUtilitiesMenuItemsCount() {
  return sizeof(utilitiesMenuItems) / sizeof(utilitiesMenuItems[0]);
}
int getInjectionToolItemsCount() {
  return sizeof(injectionToolItems) / sizeof(injectionToolItems[0]);
}
int getWifiAttackToolItemsCount() {
  return sizeof(wifiAttackToolItems) / sizeof(wifiAttackToolItems[0]);
}
int getBleAttackToolItemsCount() {
  return sizeof(bleAttackToolItems) / sizeof(bleAttackToolItems[0]);
}
int getNrfReconToolItemsCount() {
  return sizeof(nrfReconToolItems) / sizeof(nrfReconToolItems[0]);
}
int getJammingToolItemsCount() {
  return sizeof(jammingToolItems) / sizeof(jammingToolItems[0]);
}


void initializeCurrentMenu() {
  marqueeActive = false;
  marqueeOffset = 0;
  marqueeScrollLeft = true;
  gridAnimatingIn = false;

  // Reset keyboard focus for any menu that isn't password input
  if (currentMenu != WIFI_PASSWORD_INPUT) {
    keyboardFocusRow = 0;
    keyboardFocusCol = 0;
    currentKeyboardLayer = KB_LAYER_LOWERCASE;
    capsLockActive = false;
  }

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
      if (wifiIsScanning) {
        maxMenuItems = 0;  // Or 1 for a "Scanning..." message item
      } else {
        maxMenuItems = foundWifiNetworksCount + 2;  // Networks + Scan Again + Back
      }
      wifiListAnim.init();
      if (maxMenuItems > 0) {
        if (wifiMenuIndex >= maxMenuItems) wifiMenuIndex = maxMenuItems - 1;
        if (wifiMenuIndex < 0) wifiMenuIndex = 0;
      } else {
        wifiMenuIndex = 0;
      }
      wifiListAnim.setTargets(wifiMenuIndex, maxMenuItems);
      targetWifiListScrollOffset_Y = 0;  // Reset scroll on re-entry or scan finish
      currentWifiListScrollOffset_Y_anim = 0;
      break;
    case FLASHLIGHT_MODE:
      maxMenuItems = 0;  // No selectable items
      break;
    case WIFI_PASSWORD_INPUT:
      maxMenuItems = 0;  // No list items, UI is custom
      // Keyboard state init
      keyboardFocusRow = 0;
      keyboardFocusCol = 0;
      currentKeyboardLayer = KB_LAYER_LOWERCASE;
      capsLockActive = false;
      // Password buffer is expected to be initialized by caller or persist
      break;
    case WIFI_CONNECTING:
    case WIFI_CONNECTION_INFO:
      maxMenuItems = 0;  // Custom UI, no list items
      break;
  }

  if (currentMenu != WIFI_SETUP_MENU && currentMenu != WIFI_PASSWORD_INPUT && currentMenu != WIFI_CONNECTING && currentMenu != WIFI_CONNECTION_INFO) {
    if (maxMenuItems > 0) {
      if (menuIndex >= maxMenuItems) menuIndex = maxMenuItems - 1;
      if (menuIndex < 0) menuIndex = 0;
    } else if (menuIndex != 0) {
      menuIndex = 0;
    }
  }
}


void handleMenuSelection() {
  MenuState previousMenu = currentMenu;
  int localMenuIndexForNewMenu = 0;

  if (currentMenu == MAIN_MENU) {
    // ... (existing MAIN_MENU logic) ...
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
    else if (strcmp(selectedMainItem, "Info") == 0) { /* Handle Info */ return;
    }

    if (targetMenu != currentMenu) {
      currentMenu = targetMenu;
      menuIndex = localMenuIndexForNewMenu;
      initializeCurrentMenu();
    }
  } else if (currentMenu == TOOLS_MENU) {
    // ... (existing TOOLS_MENU logic) ...
    int currentMax = getToolsMenuItemsCount();
    if (menuIndex == (currentMax - 1)) {  // "Back" selected
      currentMenu = MAIN_MENU;
      for (int i = 0; i < getMainMenuItemsCount(); ++i)
        if (strcmp(mainMenuItems[i], "Tools") == 0) {
          menuIndex = i;
          mainMenuSavedIndex = i;
          break;
        }
    } else {
      toolsCategoryIndex = menuIndex;
      currentMenu = TOOL_CATEGORY_GRID;
      menuIndex = localMenuIndexForNewMenu;
    }
    initializeCurrentMenu();
  } else if (currentMenu == GAMES_MENU) {
    // ... (existing GAMES_MENU logic) ...
    int currentMax = getGamesMenuItemsCount();
    if (menuIndex == (currentMax - 1)) {  // "Back"
      currentMenu = MAIN_MENU;
      for (int i = 0; i < getMainMenuItemsCount(); ++i)
        if (strcmp(mainMenuItems[i], "Games") == 0) {
          menuIndex = i;
          mainMenuSavedIndex = i;
          break;
        }
    } else { /* Game launch logic */
    }
    if (previousMenu != currentMenu) initializeCurrentMenu();
  } else if (currentMenu == SETTINGS_MENU) {
    // ... (existing SETTINGS_MENU logic, modified for Wi-Fi Setup) ...
    int currentMax = getSettingsMenuItemsCount();
    if (menuIndex == (currentMax - 1)) {  // "Back"
      currentMenu = MAIN_MENU;
      for (int i = 0; i < getMainMenuItemsCount(); ++i)
        if (strcmp(mainMenuItems[i], "Settings") == 0) {
          menuIndex = i;
          mainMenuSavedIndex = i;
          break;
        }
    } else if (strcmp(settingsMenuItems[menuIndex], "Wi-Fi Setup") == 0) {
      currentMenu = WIFI_SETUP_MENU;
      wifiMenuIndex = localMenuIndexForNewMenu;  // Start at top of Wi-Fi list
      // Initiate scan if not already scanned or if forced
      int scanReturn = initiateAsyncWifiScan();                   // from wifi_manager
      if (scanReturn == WIFI_SCAN_RUNNING || scanReturn == -1) {  // ESP32 WiFi.scanNetworks async returns -1 for WIFI_SCAN_RUNNING
        wifiIsScanning = true;
        foundWifiNetworksCount = 0;  // Will be updated when scan finishes
        lastWifiScanCheckTime = millis();
      } else if (scanReturn >= 0) {  // Scan completed (or failed to start properly but didn't return -1/-2)
        wifiIsScanning = false;
        checkAndRetrieveWifiScanResults();  // Populate networks immediately
      } else {                              // WIFI_SCAN_FAILED (-2)
        wifiIsScanning = false;
        Serial.println("Failed to start Wi-Fi scan from settings.");
        foundWifiNetworksCount = 0;
      }
    } else { /* Other settings logic */
    }
    if (previousMenu != currentMenu) initializeCurrentMenu();

  } else if (currentMenu == TOOL_CATEGORY_GRID) {
    // ... (existing TOOL_CATEGORY_GRID logic) ...
    int currentMax = 0;
    switch (toolsCategoryIndex) {
      case 0: currentMax = getInjectionToolItemsCount(); break;
      case 1: currentMax = getWifiAttackToolItemsCount(); break;
      case 2: currentMax = getBleAttackToolItemsCount(); break;
      case 3: currentMax = getNrfReconToolItemsCount(); break;
      case 4: currentMax = getJammingToolItemsCount(); break;
      default: currentMax = 0; break;
    }
    if (menuIndex == currentMax - 1 && currentMax > 0) {
      currentMenu = TOOLS_MENU;
      menuIndex = toolsCategoryIndex;
    } else if (currentMax > 0) { /* Tool selected */
    }
    initializeCurrentMenu();
  } else if (currentMenu == WIFI_SETUP_MENU) {
    if (!wifiIsScanning) {
      if (wifiMenuIndex < foundWifiNetworksCount) {  // A network is selected
        strncpy(currentSsidToConnect, scannedNetworks[wifiMenuIndex].ssid, sizeof(currentSsidToConnect) - 1);
        currentSsidToConnect[sizeof(currentSsidToConnect) - 1] = '\0';
        selectedNetworkIsSecure = scannedNetworks[wifiMenuIndex].isSecure;

        if (selectedNetworkIsSecure) {
          currentMenu = WIFI_PASSWORD_INPUT;
          wifiPasswordInput[0] = '\0';  // Clear password buffer
          wifiPasswordInputCursor = 0;
          // Keyboard state will be set by initializeCurrentMenu
        } else {                                              // Open network
          attemptDirectWifiConnection(currentSsidToConnect);  // from wifi_manager
          currentMenu = WIFI_CONNECTING;
        }
        initializeCurrentMenu();

      } else if (wifiMenuIndex == foundWifiNetworksCount) {  // "Scan Again"
        Serial.println("Re-initiating Wi-Fi scan...");
        int scanReturn = initiateAsyncWifiScan();
        if (scanReturn == WIFI_SCAN_RUNNING || scanReturn == -1) {
          wifiIsScanning = true;
          foundWifiNetworksCount = 0;
          lastWifiScanCheckTime = millis();
        } else { /* Handle immediate completion/failure */
          wifiIsScanning = false;
          checkAndRetrieveWifiScanResults();
        }
        initializeCurrentMenu();
        wifiMenuIndex = 0;
        targetWifiListScrollOffset_Y = 0;
        currentWifiListScrollOffset_Y_anim = 0;
        if (currentMenu == WIFI_SETUP_MENU) wifiListAnim.setTargets(wifiMenuIndex, maxMenuItems);

      } else if (wifiMenuIndex == foundWifiNetworksCount + 1) {  // "Back"
        currentMenu = SETTINGS_MENU;
        for (int i = 0; i < getSettingsMenuItemsCount(); ++i)
          if (strcmp(settingsMenuItems[i], "Wi-Fi Setup") == 0) {
            menuIndex = i;
            break;
          }
        initializeCurrentMenu();
      }
    }
  } else if (currentMenu == UTILITIES_MENU) {
    // ... (existing UTILITIES_MENU logic) ...
    const char* selectedItem = utilitiesMenuItems[menuIndex];
    bool menuChanged = false;
    if (strcmp(selectedItem, "Vibration") == 0) {
      vibrationOn = !vibrationOn;
      setOutputOnPCF0(MOTOR_PIN_PCF0, vibrationOn);
    } else if (strcmp(selectedItem, "Laser") == 0) {
      laserOn = !laserOn;
      setOutputOnPCF0(LASER_PIN_PCF0, laserOn);
    } else if (strcmp(selectedItem, "Flashlight") == 0) {
      currentMenu = FLASHLIGHT_MODE;
      menuChanged = true;
    } else if (strcmp(selectedItem, "Back") == 0) {
      currentMenu = MAIN_MENU;
      menuChanged = true;
      for (int i = 0; i < getMainMenuItemsCount(); ++i)
        if (strcmp(mainMenuItems[i], "Utilities") == 0) {
          menuIndex = i;
          mainMenuSavedIndex = i;
          break;
        }
    }
    if (menuChanged) initializeCurrentMenu();
  } else if (currentMenu == FLASHLIGHT_MODE) {
    // ... (existing FLASHLIGHT_MODE selection logic - typically to go back) ...
    currentMenu = UTILITIES_MENU;
    for (int i = 0; i < getUtilitiesMenuItemsCount(); ++i)
      if (strcmp(utilitiesMenuItems[i], "Flashlight") == 0) {
        menuIndex = i;
        break;
      }
    initializeCurrentMenu();
  }
  // Note: WIFI_PASSWORD_INPUT selection (Enter key) is handled in input_handling.cpp
}

void handleMenuBackNavigation() {
  MenuState previousMenuState = currentMenu;

  if (currentMenu == MAIN_MENU) {
    return;  // Cannot go back from main menu
  } else if (currentMenu == WIFI_PASSWORD_INPUT) {
    currentMenu = WIFI_SETUP_MENU;
    wifiPasswordInput[0] = '\0';  // Clear password
    wifiPasswordInputCursor = 0;
    // wifiMenuIndex should still be on the network previously selected, or reset by initialize
  } else if (currentMenu == WIFI_CONNECTING || currentMenu == WIFI_CONNECTION_INFO) {
    // Cancel ongoing connection or clear info screen
    WiFi.disconnect();              // Stop any connection attempt
    currentMenu = WIFI_SETUP_MENU;  // Go back to the list
  } else if (currentMenu == TOOL_CATEGORY_GRID) {
    currentMenu = TOOLS_MENU;
    menuIndex = toolsCategoryIndex;
  } else if (currentMenu == WIFI_SETUP_MENU) {
    if (wifiIsScanning) {
      // Optionally cancel scan here if WiFi.scanDelete() stops it,
      // or just let it finish in background if that's preferred.
      wifiIsScanning = false;  // Stop checking scan results in main loop
    }
    currentMenu = SETTINGS_MENU;
    for (int i = 0; i < getSettingsMenuItemsCount(); ++i) {
      if (strcmp(settingsMenuItems[i], "Wi-Fi Setup") == 0) {
        menuIndex = i;
        break;
      }
    }
  } else if (currentMenu == UTILITIES_MENU) {
    currentMenu = MAIN_MENU;
    for (int i = 0; i < getMainMenuItemsCount(); ++i)
      if (strcmp(mainMenuItems[i], "Utilities") == 0) {
        menuIndex = i;
        break;
      }
    mainMenuSavedIndex = menuIndex;
  } else if (currentMenu == FLASHLIGHT_MODE) {
    currentMenu = UTILITIES_MENU;
    for (int i = 0; i < getUtilitiesMenuItemsCount(); ++i)
      if (strcmp(utilitiesMenuItems[i], "Flashlight") == 0) {
        menuIndex = i;
        break;
      }
  } else if (currentMenu == GAMES_MENU || currentMenu == TOOLS_MENU || currentMenu == SETTINGS_MENU) {
    currentMenu = MAIN_MENU;
    if (previousMenuState == GAMES_MENU)
      for (int i = 0; i < getMainMenuItemsCount(); ++i)
        if (strcmp(mainMenuItems[i], "Games") == 0) {
          mainMenuSavedIndex = i;
          break;
        } else if (previousMenuState == TOOLS_MENU)
          for (int i = 0; i < getMainMenuItemsCount(); ++i)
            if (strcmp(mainMenuItems[i], "Tools") == 0) {
              mainMenuSavedIndex = i;
              break;
            } else if (previousMenuState == SETTINGS_MENU)
              for (int i = 0; i < getMainMenuItemsCount(); ++i)
                if (strcmp(mainMenuItems[i], "Settings") == 0) {
                  mainMenuSavedIndex = i;
                  break;
                }
    menuIndex = mainMenuSavedIndex;
  }

  if (previousMenuState != currentMenu) {
    initializeCurrentMenu();
  }
}