#include "menu_logic.h"
#include "config.h"
#include "ui_drawing.h"
#include "wifi_manager.h"
#include "pcf_utils.h"
#include "keyboard_layout.h"

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

  if (currentMenu != WIFI_PASSWORD_INPUT) {
    keyboardFocusRow = 0;
    keyboardFocusCol = 0;
  }

  switch (currentMenu) {
    case MAIN_MENU:
      maxMenuItems = getMainMenuItemsCount();
      if (menuIndex >= maxMenuItems) menuIndex = maxMenuItems > 0 ? maxMenuItems -1 : 0;
      if (menuIndex < 0) menuIndex = 0;
      // ---- MODIFIED PART ----
      // Old way:
      // mainMenuAnim.init();
      // mainMenuAnim.setTargets(menuIndex, maxMenuItems);
      // New way: Start the intro animation
      // Items will start at Y-offset 0 (center of animation area) and Scale 0 (invisible)
      mainMenuAnim.startIntro(menuIndex, maxMenuItems, 0.0f, 0.0f); 
      // ---- END MODIFIED PART ----
      gridAnimatingIn = false;
      break;
    case GAMES_MENU:
      maxMenuItems = getGamesMenuItemsCount();
      if (menuIndex >= maxMenuItems) menuIndex = maxMenuItems > 0 ? maxMenuItems -1 : 0;
      if (menuIndex < 0) menuIndex = 0;
      subMenuAnim.init();
      subMenuAnim.setTargets(menuIndex, maxMenuItems);
      gridAnimatingIn = false;
      break;
    case TOOLS_MENU:
      maxMenuItems = getToolsMenuItemsCount();
      if (menuIndex >= maxMenuItems) menuIndex = maxMenuItems > 0 ? maxMenuItems -1 : 0;
      if (menuIndex < 0) menuIndex = 0;
      subMenuAnim.init();
      subMenuAnim.setTargets(menuIndex, maxMenuItems);
      gridAnimatingIn = false;
      break;
    case SETTINGS_MENU:
      maxMenuItems = getSettingsMenuItemsCount();
      if (menuIndex >= maxMenuItems) menuIndex = maxMenuItems > 0 ? maxMenuItems -1 : 0;
      if (menuIndex < 0) menuIndex = 0;
      subMenuAnim.init();
      subMenuAnim.setTargets(menuIndex, maxMenuItems);
      gridAnimatingIn = false;
      break;
    case UTILITIES_MENU:
      maxMenuItems = getUtilitiesMenuItemsCount();
      if (menuIndex >= maxMenuItems) menuIndex = maxMenuItems > 0 ? maxMenuItems -1 : 0;
      if (menuIndex < 0) menuIndex = 0;
      subMenuAnim.init();
      subMenuAnim.setTargets(menuIndex, maxMenuItems);
      gridAnimatingIn = false;
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
      if (menuIndex >= maxMenuItems) menuIndex = maxMenuItems > 0 ? maxMenuItems -1 : 0;
      if (menuIndex < 0) menuIndex = 0;
      
      targetGridScrollOffset_Y = 0;
      currentGridScrollOffset_Y_anim = 0;
      startGridItemAnimation();
      break;
    case WIFI_SETUP_MENU:
      if (!wifiHardwareEnabled) {
         maxMenuItems = 2;
      } else if (wifiIsScanning) {
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
      targetWifiListScrollOffset_Y = 0;
      currentWifiListScrollOffset_Y_anim = 0;
      gridAnimatingIn = false;
      break;
    case FLASHLIGHT_MODE:
      maxMenuItems = 0;
      gridAnimatingIn = false;
      break;
    case WIFI_PASSWORD_INPUT:
      maxMenuItems = 0;
      keyboardFocusRow = 0;
      keyboardFocusCol = 0;
      currentKeyboardLayer = KB_LAYER_LOWERCASE;
      capsLockActive = false;
      gridAnimatingIn = false;
      break;
    case WIFI_CONNECTING:
    case WIFI_CONNECTION_INFO:
      maxMenuItems = 0;
      gridAnimatingIn = false;
      break;
  }

  if (currentMenu != WIFI_SETUP_MENU && currentMenu != WIFI_PASSWORD_INPUT && 
      currentMenu != WIFI_CONNECTING && currentMenu != WIFI_CONNECTION_INFO) {
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
    if (menuIndex == (currentMax - 1)) {
      currentMenu = MAIN_MENU;
      for (int i = 0; i < getMainMenuItemsCount(); ++i) {
        if (strcmp(mainMenuItems[i], "Tools") == 0) {
          menuIndex = i;
          mainMenuSavedIndex = i;
          break;
        }
      }
    } else {
      toolsCategoryIndex = menuIndex;
      currentMenu = TOOL_CATEGORY_GRID;
      menuIndex = localMenuIndexForNewMenu;
    }
    initializeCurrentMenu();
  } else if (currentMenu == GAMES_MENU) {
    int currentMax = getGamesMenuItemsCount();
    if (menuIndex == (currentMax - 1)) {
      currentMenu = MAIN_MENU;
      for (int i = 0; i < getMainMenuItemsCount(); ++i) {
        if (strcmp(mainMenuItems[i], "Games") == 0) {
          menuIndex = i;
          mainMenuSavedIndex = i;
          break;
        }
      }
    } else {
      Serial.printf("Selected game: %s (Not yet implemented)\n", gamesMenuItems[menuIndex]);
    }
    if (previousMenu != currentMenu) {
        initializeCurrentMenu();
    }
  } else if (currentMenu == SETTINGS_MENU) {
    int currentMax = getSettingsMenuItemsCount();
    if (menuIndex == (currentMax - 1)) {
      currentMenu = MAIN_MENU;
      for (int i = 0; i < getMainMenuItemsCount(); ++i) {
        if (strcmp(mainMenuItems[i], "Settings") == 0) {
          menuIndex = i;
          mainMenuSavedIndex = i;
          break;
        }
      }
    } else if (strcmp(settingsMenuItems[menuIndex], "Wi-Fi Setup") == 0) {
      setWifiHardwareState(true);
      delay(100);

      currentMenu = WIFI_SETUP_MENU;
      wifiMenuIndex = localMenuIndexForNewMenu;
      int scanReturn = initiateAsyncWifiScan();
      
      if (scanReturn == WIFI_SCAN_RUNNING || scanReturn == -1) {
        wifiIsScanning = true;
        foundWifiNetworksCount = 0;
        lastWifiScanCheckTime = millis();
      } else if (scanReturn == -3) {
        Serial.println("Wi-Fi is disabled by manager. Cannot scan.");
        wifiIsScanning = false;
        foundWifiNetworksCount = 0;
      } else {
        wifiIsScanning = false;
        checkAndRetrieveWifiScanResults();
      }
    } else {
      Serial.printf("Selected setting: %s (Not yet implemented)\n", settingsMenuItems[menuIndex]);
    }
    if (previousMenu != currentMenu) {
        initializeCurrentMenu();
    }
  } else if (currentMenu == TOOL_CATEGORY_GRID) {
    int currentMaxItemsInGrid = 0;
    switch (toolsCategoryIndex) {
      case 0: currentMaxItemsInGrid = getInjectionToolItemsCount(); break;
      case 1: currentMaxItemsInGrid = getWifiAttackToolItemsCount(); break;
      case 2: currentMaxItemsInGrid = getBleAttackToolItemsCount(); break;
      case 3: currentMaxItemsInGrid = getNrfReconToolItemsCount(); break;
      case 4: currentMaxItemsInGrid = getJammingToolItemsCount(); break;
      default: currentMaxItemsInGrid = 0; break;
    }

    if (menuIndex == currentMaxItemsInGrid - 1 && currentMaxItemsInGrid > 0) {
      currentMenu = TOOLS_MENU;
      menuIndex = toolsCategoryIndex;
    } else if (currentMaxItemsInGrid > 0) {
      Serial.println("Tool selected from grid (Not yet implemented)");
    }
    initializeCurrentMenu();
  } else if (currentMenu == WIFI_SETUP_MENU) {
    if (!wifiHardwareEnabled) {
        if (maxMenuItems > 0 && wifiMenuIndex == maxMenuItems -1) {
             currentMenu = SETTINGS_MENU;
             for (int i = 0; i < getSettingsMenuItemsCount(); ++i)
                if (strcmp(settingsMenuItems[i], "Wi-Fi Setup") == 0) {
                    menuIndex = i; break;
                }
             initializeCurrentMenu();
        }
        return;
    }

    if (!wifiIsScanning) {
            if (wifiMenuIndex < foundWifiNetworksCount) {
        if (currentConnectedSsid.length() > 0 && strcmp(scannedNetworks[wifiMenuIndex].ssid, currentConnectedSsid.c_str()) == 0) {
            strncpy(currentSsidToConnect, scannedNetworks[wifiMenuIndex].ssid, sizeof(currentSsidToConnect) - 1);
            currentSsidToConnect[sizeof(currentSsidToConnect) - 1] = '\0';
            
            showWifiDisconnectOverlay = true;
            disconnectOverlaySelection = 0; // Default to Cancel (X)
            
            // Start animation
            disconnectOverlayAnimatingIn = true;
            disconnectOverlayCurrentScale = 0.1f; // Start from a small scale
            disconnectOverlayTargetScale = 1.0f;
            disconnectOverlayAnimStartTime = millis();
            
            return; 
        } else {
            // Not connected to this one, or no active connection; proceed with connection attempt
            strncpy(currentSsidToConnect, scannedNetworks[wifiMenuIndex].ssid, sizeof(currentSsidToConnect) - 1);
            currentSsidToConnect[sizeof(currentSsidToConnect) - 1] = '\0';
            selectedNetworkIsSecure = scannedNetworks[wifiMenuIndex].isSecure;

            KnownWifiNetwork* knownNet = findKnownNetwork(currentSsidToConnect);

            if (selectedNetworkIsSecure) {
                if (knownNet && knownNet->failCount < MAX_WIFI_FAIL_ATTEMPTS && strlen(knownNet->password) > 0) {
                    attemptWpaWifiConnection(currentSsidToConnect, knownNet->password);
                    currentMenu = WIFI_CONNECTING;
                } else {
                    currentMenu = WIFI_PASSWORD_INPUT;
                    wifiPasswordInput[0] = '\0';
                    wifiPasswordInputCursor = 0;
                }
            } else { // Open network
                attemptDirectWifiConnection(currentSsidToConnect);
                currentMenu = WIFI_CONNECTING;
            }
            initializeCurrentMenu(); // Initialize for WIFI_CONNECTING or WIFI_PASSWORD_INPUT state
        }
      } else if (wifiMenuIndex == foundWifiNetworksCount) {  // "Scan Again" selected
        Serial.println("Re-initiating Wi-Fi scan...");
        int scanReturn = initiateAsyncWifiScan();
        if (scanReturn == WIFI_SCAN_RUNNING || scanReturn == -1) {
          wifiIsScanning = true;
          foundWifiNetworksCount = 0;
          lastWifiScanCheckTime = millis();
        } else if (scanReturn == -3) {
            Serial.println("Wi-Fi is disabled by manager. Cannot rescan.");
            wifiIsScanning = false;
            foundWifiNetworksCount = 0;
        } else { 
          wifiIsScanning = false;
          checkAndRetrieveWifiScanResults();
        }
        initializeCurrentMenu();
        wifiMenuIndex = 0;
        targetWifiListScrollOffset_Y = 0;
        currentWifiListScrollOffset_Y_anim = 0;
        if (currentMenu == WIFI_SETUP_MENU) {
             wifiListAnim.setTargets(wifiMenuIndex, maxMenuItems);
        }

      } else if (wifiMenuIndex == foundWifiNetworksCount + 1) {  // "Back" selected
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
    if (menuChanged) {
        initializeCurrentMenu();
    }
  } else if (currentMenu == FLASHLIGHT_MODE) {
    currentMenu = UTILITIES_MENU;
    for (int i = 0; i < getUtilitiesMenuItemsCount(); ++i)
      if (strcmp(utilitiesMenuItems[i], "Flashlight") == 0) {
        menuIndex = i;
        break;
      }
    initializeCurrentMenu();
  }
}

void handleMenuBackNavigation() {
  MenuState previousMenuState = currentMenu;

  if (currentMenu == MAIN_MENU) {
    return;
  } else if (currentMenu == WIFI_PASSWORD_INPUT) {
    currentMenu = WIFI_SETUP_MENU;
    wifiPasswordInput[0] = '\0';
    wifiPasswordInputCursor = 0;
  } else if (currentMenu == WIFI_CONNECTING || currentMenu == WIFI_CONNECTION_INFO) {
    WiFi.disconnect();
    currentMenu = WIFI_SETUP_MENU;
    wifiMenuIndex = 0; 
    targetWifiListScrollOffset_Y = 0;
    currentWifiListScrollOffset_Y_anim = 0;
  } else if (currentMenu == TOOL_CATEGORY_GRID) {
    currentMenu = TOOLS_MENU;
    menuIndex = toolsCategoryIndex;
  } else if (currentMenu == WIFI_SETUP_MENU) {
    if (wifiIsScanning) {
      wifiIsScanning = false;
    }
    currentMenu = SETTINGS_MENU;
    for (int i = 0; i < getSettingsMenuItemsCount(); ++i) {
      if (strcmp(settingsMenuItems[i], "Wi-Fi Setup") == 0) {
        menuIndex = i;
        break;
      }
    }
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Menu Back (from Wi-Fi Setup): Not connected. Disabling Wi-Fi hardware.");
        setWifiHardwareState(false);
    }
  } else if (currentMenu == UTILITIES_MENU) {
    currentMenu = MAIN_MENU;
    for (int i = 0; i < getMainMenuItemsCount(); ++i)
      if (strcmp(mainMenuItems[i], "Utilities") == 0) {
        menuIndex = i;
        mainMenuSavedIndex = i;
        break;
      }
  } else if (currentMenu == FLASHLIGHT_MODE) {
    currentMenu = UTILITIES_MENU;
    for (int i = 0; i < getUtilitiesMenuItemsCount(); ++i)
      if (strcmp(utilitiesMenuItems[i], "Flashlight") == 0) {
        menuIndex = i;
        break;
      }
  } else if (currentMenu == GAMES_MENU || currentMenu == TOOLS_MENU || currentMenu == SETTINGS_MENU) {
    currentMenu = MAIN_MENU;
    if (previousMenuState == GAMES_MENU) {
      for (int i = 0; i < getMainMenuItemsCount(); ++i)
        if (strcmp(mainMenuItems[i], "Games") == 0) { mainMenuSavedIndex = i; break; }
    } else if (previousMenuState == TOOLS_MENU) {
      for (int i = 0; i < getMainMenuItemsCount(); ++i)
        if (strcmp(mainMenuItems[i], "Tools") == 0) { mainMenuSavedIndex = i; break; }
    } else if (previousMenuState == SETTINGS_MENU) {
      for (int i = 0; i < getMainMenuItemsCount(); ++i)
        if (strcmp(mainMenuItems[i], "Settings") == 0) { mainMenuSavedIndex = i; break; }
    }
    menuIndex = mainMenuSavedIndex;
  }

  if (previousMenuState != currentMenu) {
    initializeCurrentMenu();
  }
}