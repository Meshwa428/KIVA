#include "menu_logic.h"
#include "config.h"
#include "ui_drawing.h"
#include "wifi_manager.h"
#include "pcf_utils.h"
#include "keyboard_layout.h"
#include "jamming.h"
#include "ota_manager.h"
#include "firmware_metadata.h"  // For availableSdFirmwareCount

// --- Menu Item Arrays ---
const char* mainMenuItems[] = {
  "Tools", "Games", "Settings", "Utilities", "Info"
};
const char* gamesMenuItems[] = { "Snake Game", "Tetris Clone", "Classic Pong", "2D Maze", "Back" };
// MODIFIED LINE: Added "Firmware Update"
const char* settingsMenuItems[] = { "Wi-Fi Setup", "Display Opts", "Sound Setup", "Firmware Update", "System Info", "About Kiva", "Back" };
const char* utilitiesMenuItems[] = { "Vibration", "Laser", "Flashlight", "Back" };
const char* toolsMenuItems[] = { "Injection", "Wi-Fi Attacks", "BLE/BT Attacks", "NRF Recon", "Jamming", "Back" };  // Jamming is index 4
const char* injectionToolItems[] = { "MouseJack", "Wireless Keystroke", "Fake HID", "BLE HID Inject", "RF Payload", "Back" };
const char* wifiAttackToolItems[] = { "Beacon Spam", "Deauth Attack", "Probe Flood", "Fake AP", "Karma AP", "DNS Spoof", "Packet Inject", "Packet Replay", "Back" };
const char* bleAttackToolItems[] = { "BLE Ad Spam", "BLE Scanner", "BLE DoS", "BLE Name Spoof", "BT MAC Spoof", "Back" };
const char* nrfReconToolItems[] = { "NRF Scanner", "ShockBurst Sniff", "Key Sniffer", "NRF Brute", "Custom Flood", "RF Spammer", "BLE Ad Disrupt", "Dual Interference", "Back" };
const char* jammingToolItems[] = {
  "BLE Jam",          // Index 0
  "BT Jam",           // Index 1
  "NRF Channel Jam",  // Index 2
  "RF Noise Flood",   // Index 3
  "Back"              // Index 4
};
const char* otaMenuItems[] = {  // <--- NEW ARRAY
  "Update via Web", "Update from SD", "Basic OTA (IDE)", "Back"
};

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
int getOtaMenuItemsCount() {
  return sizeof(otaMenuItems) / sizeof(otaMenuItems[0]);
}  // <--- NEW GETTER


void initializeCurrentMenu() {
  marqueeActive = false;
  marqueeOffset = 0;
  marqueeScrollLeft = true;

  if (currentMenu != WIFI_PASSWORD_INPUT) {
    keyboardFocusRow = 0;
    keyboardFocusCol = 0;
  }
  if (currentMenu == FIRMWARE_UPDATE_GRID) {
    if (otaProgress > 0 && otaProgress < 100) {
      Serial.println("WARN: Initializing FIRMWARE_UPDATE_GRID but otaProgress suggests active OTA! Forcing reset.");
      resetOtaState();  // Force reset if in an inconsistent state
    } else if (otaProgress == 100 && otaStatusMessage.indexOf("Rebooting") == -1) {
      // Update was done, but not rebooting yet, and we are back to selection. Reset.
      resetOtaState();
    }
    // If otaProgress is -1 (error) or -3 (already up to date), these are fine, don't reset.
    // If otaProgress is -2 (idle), it's also fine.
  }


  if (!isJammingOperationActive && currentMenu != OTA_WEB_ACTIVE && currentMenu != OTA_BASIC_ACTIVE && currentMenu != OTA_SD_STATUS) {
    currentBatteryCheckInterval = BATTERY_CHECK_INTERVAL_NORMAL;
    currentInputPollInterval = INPUT_POLL_INTERVAL_NORMAL;
  } else if (currentMenu == OTA_WEB_ACTIVE || currentMenu == OTA_BASIC_ACTIVE || currentMenu == OTA_SD_STATUS) {
    currentInputPollInterval = INPUT_POLL_INTERVAL_JAMMING;
  }

  switch (currentMenu) {
    case MAIN_MENU: maxMenuItems = getMainMenuItemsCount(); break;
    case GAMES_MENU: maxMenuItems = getGamesMenuItemsCount(); break;
    case TOOLS_MENU: maxMenuItems = getToolsMenuItemsCount(); break;
    case SETTINGS_MENU: maxMenuItems = getSettingsMenuItemsCount(); break;
    case UTILITIES_MENU: maxMenuItems = getUtilitiesMenuItemsCount(); break;
    case FIRMWARE_UPDATE_GRID: maxMenuItems = getOtaMenuItemsCount(); break;
    case FIRMWARE_SD_LIST_MENU:
      maxMenuItems = availableSdFirmwareCount > 0 ? availableSdFirmwareCount + 1 : 1;
      break;
    case TOOL_CATEGORY_GRID:
      if (toolsCategoryIndex >= 0 && toolsCategoryIndex < getToolsMenuItemsCount() && strcmp(toolsMenuItems[toolsCategoryIndex], "Jamming") == 0) {
        maxMenuItems = getJammingToolItemsCount();
      } else if (toolsCategoryIndex >= 0 && toolsCategoryIndex < getToolsMenuItemsCount()) {
        switch (toolsCategoryIndex) {
          case 0: maxMenuItems = getInjectionToolItemsCount(); break;
          case 1: maxMenuItems = getWifiAttackToolItemsCount(); break;
          case 2: maxMenuItems = getBleAttackToolItemsCount(); break;
          case 3: maxMenuItems = getNrfReconToolItemsCount(); break;
          default: maxMenuItems = 0; break;
        }
      } else {
        maxMenuItems = 0;
      }
      break;
    case WIFI_SETUP_MENU:
      if (!wifiHardwareEnabled) {
        maxMenuItems = 2;
      } else if (wifiIsScanning) {
        maxMenuItems = 0;
      } else {
        maxMenuItems = foundWifiNetworksCount + 2;
      }
      break;
    case FLASHLIGHT_MODE:
    case WIFI_PASSWORD_INPUT:
    case WIFI_CONNECTING:
    case WIFI_CONNECTION_INFO:
    case JAMMING_ACTIVE_SCREEN:
    case OTA_WEB_ACTIVE:
    case OTA_SD_STATUS:
    case OTA_BASIC_ACTIVE:
      maxMenuItems = 0;
      break;
    default: maxMenuItems = 0; break;
  }

  if (currentMenu == WIFI_SETUP_MENU || currentMenu == FIRMWARE_SD_LIST_MENU) {
    if (maxMenuItems > 0) {
      wifiMenuIndex = constrain(wifiMenuIndex, 0, maxMenuItems - 1);
    } else {
      wifiMenuIndex = 0;
    }
  } else {
    if (maxMenuItems > 0) {
      menuIndex = constrain(menuIndex, 0, maxMenuItems - 1);
    } else {
      menuIndex = 0;
    }
  }

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
    case FIRMWARE_UPDATE_GRID:
      targetGridScrollOffset_Y = 0;
      currentGridScrollOffset_Y_anim = 0;
      startGridItemAnimation();
      break;
    case WIFI_SETUP_MENU:
    case FIRMWARE_SD_LIST_MENU:
      wifiListAnim.init();
      wifiListAnim.setTargets(wifiMenuIndex, maxMenuItems);
      targetWifiListScrollOffset_Y = 0;
      currentWifiListScrollOffset_Y_anim = 0;
      gridAnimatingIn = false;
      break;
    case FLASHLIGHT_MODE:
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
    case OTA_WEB_ACTIVE:
    case OTA_SD_STATUS:
    case OTA_BASIC_ACTIVE:
      gridAnimatingIn = false;
      break;
  }
}

void handleMenuSelection() {
  MenuState originalMenu = currentMenu;
  int newMenuIndexForSubMenu = 0;
  bool needsReinit = false;

  if (currentMenu == MAIN_MENU) {
    mainMenuSavedIndex = menuIndex;
    MenuState targetMenu = MAIN_MENU;
    if (menuIndex >= 0 && menuIndex < getMainMenuItemsCount()) {
      const char* selectedItem = mainMenuItems[menuIndex];
      if (strcmp(selectedItem, "Tools") == 0) targetMenu = TOOLS_MENU;
      else if (strcmp(selectedItem, "Games") == 0) targetMenu = GAMES_MENU;
      else if (strcmp(selectedItem, "Settings") == 0) targetMenu = SETTINGS_MENU;
      else if (strcmp(selectedItem, "Utilities") == 0) targetMenu = UTILITIES_MENU;
      else if (strcmp(selectedItem, "Info") == 0) {
        Serial.println("Info selected (NYI)");
        return;
      }
    }
    if (targetMenu != currentMenu) {
      currentMenu = targetMenu;
      menuIndex = newMenuIndexForSubMenu;
      needsReinit = true;
    }
  } else if (currentMenu == TOOLS_MENU) {
    if (menuIndex == getToolsMenuItemsCount() - 1) {
      currentMenu = MAIN_MENU;
      menuIndex = mainMenuSavedIndex;
    } else {
      toolsCategoryIndex = menuIndex;
      currentMenu = TOOL_CATEGORY_GRID;
      menuIndex = newMenuIndexForSubMenu;
    }
    needsReinit = true;
  } else if (currentMenu == GAMES_MENU) {
    if (menuIndex == getGamesMenuItemsCount() - 1) {
      currentMenu = MAIN_MENU;
      menuIndex = mainMenuSavedIndex;
      needsReinit = true;
    } else {
      Serial.printf("Game: %s (NYI)\n", gamesMenuItems[menuIndex]);
      return;
    }
  } else if (currentMenu == SETTINGS_MENU) {
    if (menuIndex == getSettingsMenuItemsCount() - 1) {
      currentMenu = MAIN_MENU;
      menuIndex = mainMenuSavedIndex;
      needsReinit = true;
    } else if (strcmp(settingsMenuItems[menuIndex], "Wi-Fi Setup") == 0) {
      // PHASE 2: Prompt for Wi-Fi will go here
      setWifiHardwareState(true, WIFI_MODE_STA);  // Explicitly request STA mode
      delay(100);
      currentMenu = WIFI_SETUP_MENU;
      wifiMenuIndex = newMenuIndexForSubMenu;
      int scanReturn = initiateAsyncWifiScan();
      wifiIsScanning = (scanReturn == WIFI_SCAN_RUNNING || scanReturn == -1);
      if (wifiIsScanning) {
        foundWifiNetworksCount = 0;
        lastWifiScanCheckTime = millis();
      } else if (scanReturn != -3) {
        checkAndRetrieveWifiScanResults();
      }
      needsReinit = true;
    } else if (strcmp(settingsMenuItems[menuIndex], "Firmware Update") == 0) {
      if (isJammingOperationActive) {
        otaStatusMessage = "Disable Jammer!";
        Serial.println("MenuLogic: " + otaStatusMessage);
        return;
      }
      // CRITICAL: Reset OTA state when entering the selection grid
      resetOtaState();
      currentMenu = FIRMWARE_UPDATE_GRID;
      menuIndex = newMenuIndexForSubMenu;
      needsReinit = true;
    } else {
      Serial.printf("Setting: %s (NYI)\n", settingsMenuItems[menuIndex]);
      return;
    }
  } else if (currentMenu == FIRMWARE_UPDATE_GRID) {
    const char* selectedOtaItem = otaMenuItems[menuIndex];
    MenuState nextMenuTarget = currentMenu;
    bool otaStartAttempted = false;

    if (strcmp(selectedOtaItem, "Update via Web") == 0) {
      otaStartAttempted = true;
      // PHASE 2: Wi-Fi Redirect Prompt will go here
      if (startWebOtaUpdate()) {
        nextMenuTarget = OTA_WEB_ACTIVE;
      }  // otaStatusMessage is set by startWebOtaUpdate
    } else if (strcmp(selectedOtaItem, "Update from SD") == 0) {
      otaStartAttempted = true;
      nextMenuTarget = FIRMWARE_SD_LIST_MENU;
      wifiMenuIndex = newMenuIndexForSubMenu;
    } else if (strcmp(selectedOtaItem, "Basic OTA (IDE)") == 0) {
      otaStartAttempted = true;
      nextMenuTarget = OTA_BASIC_ACTIVE;
    } else if (strcmp(selectedOtaItem, "Back") == 0) {
      nextMenuTarget = SETTINGS_MENU;
      for (int i = 0; i < getSettingsMenuItemsCount(); ++i) {
        if (strcmp(settingsMenuItems[i], "Firmware Update") == 0) {
          menuIndex = i;
          break;
        }
      }
    }

    if (nextMenuTarget != currentMenu) {
      currentMenu = nextMenuTarget;
      needsReinit = true;
    } else if (otaStartAttempted) {
      // If an OTA start was attempted but failed (nextMenuTarget didn't change),
      // we are still in FIRMWARE_UPDATE_GRID. We need to ensure the UI
      // can display the otaStatusMessage from the failed attempt.
      // A full re-initialization might reset animations, which is fine.
      // The key is that otaStatusMessage is preserved by startXxxOtaUpdate functions.
      needsReinit = true;
    }
  } else if (currentMenu == FIRMWARE_SD_LIST_MENU) {
    if (availableSdFirmwareCount > 0 && wifiMenuIndex < availableSdFirmwareCount) {
      g_firmwareToInstallFromSd = availableSdFirmwares[wifiMenuIndex]; // Store the selection

      resetOtaState(); // Clean slate before setting "Preparing"
      otaStatusMessage = "Preparing: " + String(g_firmwareToInstallFromSd.version).substring(0,15);
      otaProgress = 5; // Small progress indicating preparation (e.g., "Preparing")
      
      currentMenu = OTA_SD_STATUS; // Switch to the status screen
      // Set a flag or a sub-state for OTA_SD_STATUS to indicate "pending actual update"
      // Let's use a specific otaProgress value for this phase, e.g., otaProgress = 1 (PENDING_SD_UPDATE_START)
      otaProgress = OTA_PROGRESS_PENDING_SD_START; // Define this as 1, for example.

    } else { // "Back" was selected or invalid index
      currentMenu = FIRMWARE_UPDATE_GRID;
      for (int i = 0; i < getOtaMenuItemsCount(); ++i) {
        if (strcmp(otaMenuItems[i], "Update from SD") == 0) {
          menuIndex = i; // Select "Update from SD" in the previous menu
          break;
        }
      }
    }
    needsReinit = true; // Reinitialize the new menu
  } else if (currentMenu == TOOL_CATEGORY_GRID) {
    int currentMaxItemsInGrid = 0;
    bool isJammingCat = false;
    const char** currentToolItemsListPtr = nullptr;
    if (toolsCategoryIndex >= 0 && toolsCategoryIndex < getToolsMenuItemsCount()) {
      if (strcmp(toolsMenuItems[toolsCategoryIndex], "Jamming") == 0) {
        isJammingCat = true;
        currentMaxItemsInGrid = getJammingToolItemsCount();
        currentToolItemsListPtr = jammingToolItems;
      } else { /* other categories */
        switch (toolsCategoryIndex) {
          case 0:
            currentMaxItemsInGrid = getInjectionToolItemsCount();
            currentToolItemsListPtr = injectionToolItems;
            break;
          case 1:
            currentMaxItemsInGrid = getWifiAttackToolItemsCount();
            currentToolItemsListPtr = wifiAttackToolItems;
            break;
          case 2:
            currentMaxItemsInGrid = getBleAttackToolItemsCount();
            currentToolItemsListPtr = bleAttackToolItems;
            break;
          case 3:
            currentMaxItemsInGrid = getNrfReconToolItemsCount();
            currentToolItemsListPtr = nrfReconToolItems;
            break;
          default: currentMaxItemsInGrid = 0; break;
        }
      }
    }
    if (menuIndex == currentMaxItemsInGrid - 1 && currentMaxItemsInGrid > 0) {
      currentMenu = TOOLS_MENU;
      menuIndex = toolsCategoryIndex;
      needsReinit = true;
    } else if (isJammingCat && menuIndex >= 0 && menuIndex < (currentMaxItemsInGrid - 1)) {
      lastSelectedJammingToolGridIndex = menuIndex;
      JammingType jamTypeToStart = JAM_NONE;
      if (currentToolItemsListPtr) {
        const char* selectedJamItem = currentToolItemsListPtr[menuIndex];
        if (strcmp(selectedJamItem, "BLE Jam") == 0) jamTypeToStart = JAM_BLE;
        else if (strcmp(selectedJamItem, "BT Jam") == 0) jamTypeToStart = JAM_BT;
        else if (strcmp(selectedJamItem, "NRF Channel Jam") == 0) jamTypeToStart = JAM_NRF_CHANNELS;
        else if (strcmp(selectedJamItem, "RF Noise Flood") == 0) jamTypeToStart = JAM_RF_NOISE_FLOOD;
      }
      if (jamTypeToStart != JAM_NONE && startActiveJamming(jamTypeToStart)) {
        currentMenu = JAMMING_ACTIVE_SCREEN;
        needsReinit = true;
      } else if (jamTypeToStart != JAM_NONE) {
        Serial.println("Failed to start jamming");
        return;
      }
    } else if (!isJammingCat && currentMaxItemsInGrid > 0 && menuIndex >= 0 && menuIndex < (currentMaxItemsInGrid - 1)) {
      const char* catName = (toolsCategoryIndex >= 0 && toolsCategoryIndex < getToolsMenuItemsCount()) ? toolsMenuItems[toolsCategoryIndex] : "Unknown Cat";
      const char* itemName = (currentToolItemsListPtr && menuIndex < currentMaxItemsInGrid) ? currentToolItemsListPtr[menuIndex] : "Unknown Item";
      Serial.printf("Tool '%s' from cat '%s' (NYI)\n", itemName, catName);
      return;
    } else {
      return;
    }
  } else if (currentMenu == WIFI_SETUP_MENU) {
    MenuState nextWiFiMenu = currentMenu;
    bool wifiMenuNeedsReinit = false;

    if (!wifiHardwareEnabled) {
      if (maxMenuItems > 0 && wifiMenuIndex == (maxMenuItems - 1)) {  // Back
        nextWiFiMenu = SETTINGS_MENU;
        for (int i = 0; i < getSettingsMenuItemsCount(); ++i)
          if (strcmp(settingsMenuItems[i], "Wi-Fi Setup") == 0) {
            menuIndex = i;
            break;
          }
      } else if (maxMenuItems > 0 && wifiMenuIndex == 0) {  // "Enable Wi-Fi"
        setWifiHardwareState(true, WIFI_MODE_STA);
        delay(100);
        int scanReturn = initiateAsyncWifiScan();
        wifiIsScanning = (scanReturn == WIFI_SCAN_RUNNING || scanReturn == -1);
        if (wifiIsScanning) {
          foundWifiNetworksCount = 0;
          lastWifiScanCheckTime = millis();
        } else if (scanReturn != -3) {
          checkAndRetrieveWifiScanResults();
        }
        wifiMenuIndex = newMenuIndexForSubMenu;
        wifiMenuNeedsReinit = true;  // Re-init for scan status
      }
    } else if (!wifiIsScanning) {
      if (wifiMenuIndex < foundWifiNetworksCount) {  // A network is selected
        if (currentConnectedSsid.length() > 0 && strcmp(scannedNetworks[wifiMenuIndex].ssid, currentConnectedSsid.c_str()) == 0) {
          strncpy(currentSsidToConnect, scannedNetworks[wifiMenuIndex].ssid, sizeof(currentSsidToConnect) - 1);
          currentSsidToConnect[sizeof(currentSsidToConnect) - 1] = '\0';
          showWifiDisconnectOverlay = true;
          disconnectOverlaySelection = 0;
          disconnectOverlayAnimatingIn = true;
          disconnectOverlayCurrentScale = 0.1f;
          disconnectOverlayTargetScale = 1.0f;
          disconnectOverlayAnimStartTime = millis();
          return;  // Overlay handles next steps
        } else {   // Connect to a new network
          strncpy(currentSsidToConnect, scannedNetworks[wifiMenuIndex].ssid, sizeof(currentSsidToConnect) - 1);
          currentSsidToConnect[sizeof(currentSsidToConnect) - 1] = '\0';
          selectedNetworkIsSecure = scannedNetworks[wifiMenuIndex].isSecure;
          KnownWifiNetwork* knownNet = findKnownNetwork(currentSsidToConnect);
          if (selectedNetworkIsSecure) {
            if (knownNet && knownNet->failCount < MAX_WIFI_FAIL_ATTEMPTS && strlen(knownNet->password) > 0) {
              attemptWpaWifiConnection(currentSsidToConnect, knownNet->password);
              nextWiFiMenu = WIFI_CONNECTING;
            } else {
              nextWiFiMenu = WIFI_PASSWORD_INPUT;
              wifiPasswordInput[0] = '\0';
              wifiPasswordInputCursor = 0;
            }
          } else {
            attemptDirectWifiConnection(currentSsidToConnect);
            nextWiFiMenu = WIFI_CONNECTING;
          }
        }
      } else if (wifiMenuIndex == foundWifiNetworksCount) {  // "Scan Again" selected
        int scanReturn = initiateAsyncWifiScan();
        wifiIsScanning = (scanReturn == WIFI_SCAN_RUNNING || scanReturn == -1);
        if (wifiIsScanning) {
          foundWifiNetworksCount = 0;
          lastWifiScanCheckTime = millis();
        } else if (scanReturn != -3) {
          checkAndRetrieveWifiScanResults();
        }
        wifiMenuIndex = newMenuIndexForSubMenu;
        targetWifiListScrollOffset_Y = 0;
        currentWifiListScrollOffset_Y_anim = 0;
        wifiMenuNeedsReinit = true;                              // Re-init for scan status
      } else if (wifiMenuIndex == foundWifiNetworksCount + 1) {  // "Back" selected
        nextWiFiMenu = SETTINGS_MENU;
        for (int i = 0; i < getSettingsMenuItemsCount(); ++i)
          if (strcmp(settingsMenuItems[i], "Wi-Fi Setup") == 0) {
            menuIndex = i;
            break;
          }
      }
    }
    if (nextWiFiMenu != currentMenu) {
      currentMenu = nextWiFiMenu;
      needsReinit = true;
    } else if (wifiMenuNeedsReinit) {
      needsReinit = true;
    }  // If stayed in WiFi setup but state changed (e.g. scan started)
  } else if (currentMenu == UTILITIES_MENU) {
    const char* selectedItem = utilitiesMenuItems[menuIndex];
    if (strcmp(selectedItem, "Vibration") == 0) {
      vibrationOn = !vibrationOn;
      setOutputOnPCF0(MOTOR_PIN_PCF0, vibrationOn);
      return;
    } else if (strcmp(selectedItem, "Laser") == 0) {
      laserOn = !laserOn;
      setOutputOnPCF0(LASER_PIN_PCF0, laserOn);
      return;
    } else if (strcmp(selectedItem, "Flashlight") == 0) {
      currentMenu = FLASHLIGHT_MODE;
      needsReinit = true;
    } else if (strcmp(selectedItem, "Back") == 0) {
      currentMenu = MAIN_MENU;
      for (int i = 0; i < getMainMenuItemsCount(); ++i)
        if (strcmp(mainMenuItems[i], "Utilities") == 0) {
          menuIndex = i;
          mainMenuSavedIndex = i;
          break;
        }
      needsReinit = true;
    }
  } else if (currentMenu == FLASHLIGHT_MODE) {
    currentMenu = UTILITIES_MENU;
    for (int i = 0; i < getUtilitiesMenuItemsCount(); ++i)
      if (strcmp(utilitiesMenuItems[i], "Flashlight") == 0) {
        menuIndex = i;
        break;
      }
    needsReinit = true;
  }

  if (needsReinit) {  // Call initializeCurrentMenu if menu changed OR explicitly flagged
    initializeCurrentMenu();
  }
}

// Function: handleMenuBackNavigation() - Revised to ensure ota state reset
void handleMenuBackNavigation() {
  MenuState previousMenuState = currentMenu;
  bool menuChanged = true;

  // When backing out of an OTA process or related menu, ensure OTA state is reset.
  if (currentMenu == OTA_WEB_ACTIVE) {
    stopWebOtaUpdate();
    currentMenu = FIRMWARE_UPDATE_GRID;
    menuIndex = 0;
  } else if (currentMenu == OTA_SD_STATUS) {
    resetOtaState();
    currentMenu = FIRMWARE_SD_LIST_MENU;
    wifiMenuIndex = 0;
  } else if (currentMenu == OTA_BASIC_ACTIVE) {
    stopBasicOtaUpdate();
    currentMenu = FIRMWARE_UPDATE_GRID;
    menuIndex = 2;
  } else if (currentMenu == FIRMWARE_SD_LIST_MENU) {
    resetOtaState();  // Reset when going from list back to OTA selection grid
    currentMenu = FIRMWARE_UPDATE_GRID;
    for (int i = 0; i < getOtaMenuItemsCount(); ++i) {
      if (strcmp(otaMenuItems[i], "Update from SD") == 0) {
        menuIndex = i;
        break;
      }
    }
  } else if (currentMenu == FIRMWARE_UPDATE_GRID) {
    resetOtaState();  // Reset when going from OTA selection grid back to Settings
    currentMenu = SETTINGS_MENU;
    for (int i = 0; i < getSettingsMenuItemsCount(); ++i) {
      if (strcmp(settingsMenuItems[i], "Firmware Update") == 0) {
        menuIndex = i;
        break;
      }
    }
  } else if (currentMenu == JAMMING_ACTIVE_SCREEN) {
    stopActiveJamming();
    currentMenu = TOOL_CATEGORY_GRID;
    bool foundJammingCat = false;
    for (int i = 0; i < getToolsMenuItemsCount(); ++i) {
      if (strcmp(toolsMenuItems[i], "Jamming") == 0) {
        toolsCategoryIndex = i;
        foundJammingCat = true;
        break;
      }
    }
    if (!foundJammingCat) toolsCategoryIndex = 4;
    menuIndex = lastSelectedJammingToolGridIndex;
    int numJammingItems = getJammingToolItemsCount();
    if (numJammingItems > 0) menuIndex = constrain(menuIndex, 0, numJammingItems - 1);
    else menuIndex = 0;
  } else if (currentMenu == MAIN_MENU) {
    menuChanged = false;
  } else if (currentMenu == WIFI_PASSWORD_INPUT) {
    currentMenu = WIFI_SETUP_MENU;
    wifiPasswordInput[0] = '\0';
    wifiPasswordInputCursor = 0;
  } else if (currentMenu == WIFI_CONNECTING || currentMenu == WIFI_CONNECTION_INFO) {
    if (WiFi.status() != WL_NO_SHIELD && WiFi.status() != WL_IDLE_STATUS) {
      WiFi.disconnect(true);
      delay(100);
    }
    currentMenu = WIFI_SETUP_MENU;
    wifiMenuIndex = 0;
    targetWifiListScrollOffset_Y = 0;
    currentWifiListScrollOffset_Y_anim = 0;
  } else if (currentMenu == TOOL_CATEGORY_GRID) {
    currentMenu = TOOLS_MENU;
    menuIndex = toolsCategoryIndex;
    if (isJammingOperationActive) { stopActiveJamming(); }
  } else if (currentMenu == WIFI_SETUP_MENU) {
    if (wifiIsScanning) {
      wifiIsScanning = false;
      WiFi.scanDelete();
    }
    currentMenu = SETTINGS_MENU;
    for (int i = 0; i < getSettingsMenuItemsCount(); ++i)
      if (strcmp(settingsMenuItems[i], "Wi-Fi Setup") == 0) {
        menuIndex = i;
        break;
      }
    if (WiFi.status() != WL_CONNECTED && wifiHardwareEnabled) { setWifiHardwareState(false); }
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
    if (previousMenuState != FIRMWARE_UPDATE_GRID && previousMenuState != FIRMWARE_SD_LIST_MENU && previousMenuState != OTA_WEB_ACTIVE && previousMenuState != OTA_SD_STATUS && previousMenuState != OTA_BASIC_ACTIVE) {
      currentMenu = MAIN_MENU;
      if (previousMenuState == GAMES_MENU) {
        for (int i = 0; i < getMainMenuItemsCount(); ++i)
          if (strcmp(mainMenuItems[i], "Games") == 0) {
            mainMenuSavedIndex = i;
            break;
          }
      } else if (previousMenuState == TOOLS_MENU) {
        for (int i = 0; i < getMainMenuItemsCount(); ++i)
          if (strcmp(mainMenuItems[i], "Tools") == 0) {
            mainMenuSavedIndex = i;
            break;
          }
      } else if (previousMenuState == SETTINGS_MENU) {
        for (int i = 0; i < getMainMenuItemsCount(); ++i)
          if (strcmp(mainMenuItems[i], "Settings") == 0) {
            mainMenuSavedIndex = i;
            break;
          }
      }
      menuIndex = mainMenuSavedIndex;
    } else {
      menuChanged = false;
    }
  } else {
    menuChanged = false;
  }

  if (menuChanged) {
    initializeCurrentMenu();
  }
}