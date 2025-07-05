#include "menu_logic.h"
#include "config.h"
#include "ui_drawing.h"
#include "wifi_manager.h"
#include "pcf_utils.h"
#include "keyboard_layout.h"
#include "jamming.h"
#include "wifi_attack_tools.h"
#include "ota_manager.h"
#include "firmware_metadata.h"  // For availableSdFirmwareCount

// --- Menu Item Arrays ---
const char* mainMenuItems[] = {
  "Tools", "Games", "Settings", "Utilities", "Info"  // <--- MODIFIED: Added "RF Toolkit"
};
const char* gamesMenuItems[] = { "Snake Game", "Tetris Clone", "Classic Pong", "2D Maze", "Back" };
const char* settingsMenuItems[] = { "Wi-Fi Setup", "Display Opts", "Sound Setup", "Firmware Update", "System Info", "Back" };
const char* utilitiesMenuItems[] = { "Vibration", "Laser", "Flashlight", "Back" };
const char* toolsMenuItems[] = { "Injection", "Wi-Fi Attacks", "BLE/BT Attacks", "NRF Recon", "Jamming", "Back" };  // Jamming is index 4
const char* injectionToolItems[] = { "MouseJack", "Wireless Keystroke", "Fake HID", "BLE HID Inject", "RF Payload", "Back" };
const char* wifiAttackToolItems[] = {
  "Beacon Spam", "Rick Roll Spam", "Deauth Attack", "Probe Flood", "Fake AP",  // <--- ADDED "Rick Roll Spam"
  "Karma AP", "DNS Spoof", "Packet Inject", "Packet Replay", "Back"
};
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
  return sizeof(gamesMenuItems) / sizeof(gamesMenuItems[0]);  // This line
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

  if (currentMenu == WIFI_PASSWORD_INPUT) {
    currentKeyboardLayer = KB_LAYER_LOWERCASE;
    capsLockActive = false;
    keyboardFocusRow = 0;
    keyboardFocusCol = 0;
    // wifiPasswordInput is cleared when entering WIFI_PASSWORD_INPUT from WIFI_SETUP_MENU
    // in handleMenuSelection.
  }

  if (currentMenu == FIRMWARE_UPDATE_GRID) {
    if (otaProgress > 0 && otaProgress < 100) {
      Serial.println("WARN: Initializing FIRMWARE_UPDATE_GRID but otaProgress suggests active OTA! Forcing reset.");
      resetOtaState();
    } else if (otaProgress == 100 && otaStatusMessage.indexOf("Rebooting") == -1) {
      resetOtaState();
    }
  }

  if (isJammingOperationActive) {
    currentBatteryCheckInterval = BATTERY_CHECK_INTERVAL_JAMMING;
    currentInputPollInterval = INPUT_POLL_INTERVAL_JAMMING;
  } else {
    currentBatteryCheckInterval = BATTERY_CHECK_INTERVAL_NORMAL;
    currentInputPollInterval = INPUT_POLL_INTERVAL_NORMAL;
  }


  // Updated radio interval logic based on currentRfMode
  if (currentRfMode == RF_MODE_NRF_JAMMING) {  // NRF Jamming takes precedence
    currentBatteryCheckInterval = BATTERY_CHECK_INTERVAL_JAMMING;
    currentInputPollInterval = INPUT_POLL_INTERVAL_JAMMING;
  } else if (currentRfMode == RF_MODE_WIFI_SNIFF_PROMISC || currentRfMode == RF_MODE_WIFI_INJECT_AP) {
    // Potentially different intervals for Wi-Fi sniffing/injection if needed
    currentBatteryCheckInterval = BATTERY_CHECK_INTERVAL_JAMMING;  // Example: reuse jamming interval
    currentInputPollInterval = INPUT_POLL_INTERVAL_NORMAL;         // Sniffing might need faster input
  } else {
    currentBatteryCheckInterval = BATTERY_CHECK_INTERVAL_NORMAL;
    currentInputPollInterval = INPUT_POLL_INTERVAL_NORMAL;
  }


  switch (currentMenu) {
    case MAIN_MENU: maxMenuItems = getMainMenuItemsCount(); break;
    case GAMES_MENU: maxMenuItems = getGamesMenuItemsCount(); break;
    case TOOLS_MENU: maxMenuItems = getToolsMenuItemsCount(); break;
    case SETTINGS_MENU: maxMenuItems = getSettingsMenuItemsCount(); break;
    case UTILITIES_MENU: maxMenuItems = getUtilitiesMenuItemsCount(); break;
    case FIRMWARE_UPDATE_GRID: maxMenuItems = getOtaMenuItemsCount(); break;
    case FIRMWARE_SD_LIST_MENU:
      prepareSdFirmwareList();
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
        maxMenuItems = 2;  // "Enable Wi-Fi", "Back"
      } else if (wifiIsScanning) {
        maxMenuItems = 0;  // Display "Scanning..." message, no list items, drawWifiSetupScreen will handle text
      } else {
        maxMenuItems = foundWifiNetworksCount + 2;  // Networks + "Scan Again" + "Back"
      }
      break;
    case FLASHLIGHT_MODE:
    case WIFI_PASSWORD_INPUT:  // Max items is 0, keyboard state handled at top
    case WIFI_CONNECTING:
    case WIFI_CONNECTION_INFO:
    case JAMMING_ACTIVE_SCREEN:
    case OTA_WEB_ACTIVE:
    case OTA_SD_STATUS:
    case OTA_BASIC_ACTIVE:
      maxMenuItems = 0;
      break;
    case WIFI_BEACON_SPAM_ACTIVE_SCREEN:
      maxMenuItems = 0;
      break;
    case WIFI_RICK_ROLL_ACTIVE_SCREEN:
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
      if (maxMenuItems > 0) {  // Only set targets if there are items (not in "Scanning..." state)
        wifiListAnim.setTargets(wifiMenuIndex, maxMenuItems);
      }
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
    case WIFI_BEACON_SPAM_ACTIVE_SCREEN:  // <--- ADD THIS CASE
      gridAnimatingIn = false;
      break;
    case WIFI_RICK_ROLL_ACTIVE_SCREEN:  // <--- ADD THIS CASE
      gridAnimatingIn = false;
      break;
  }
}

// Add this function, e.g., in menu_logic.cpp
void activatePromptOverlay(const char* title, const char* message,
                           const char* opt0Txt, const char* opt1Txt,
                           MenuState actionMenu,
                           void (*confirmAction)()) {
  strncpy(promptOverlayTitle, title, sizeof(promptOverlayTitle) - 1);
  promptOverlayTitle[sizeof(promptOverlayTitle) - 1] = '\0';
  strncpy(promptOverlayMessage, message, sizeof(promptOverlayMessage) - 1);
  promptOverlayMessage[sizeof(promptOverlayMessage) - 1] = '\0';
  strncpy(promptOverlayOption0Text, opt0Txt, sizeof(promptOverlayOption0Text) - 1);
  promptOverlayOption0Text[sizeof(promptOverlayOption0Text) - 1] = '\0';
  strncpy(promptOverlayOption1Text, opt1Txt, sizeof(promptOverlayOption1Text) - 1);
  promptOverlayOption1Text[sizeof(promptOverlayOption1Text) - 1] = '\0';

  promptOverlayActionMenuTarget = actionMenu;
  promptOverlayConfirmAction = confirmAction;

  promptOverlaySelection = 0;
  showPromptOverlay = true;
  promptOverlayAnimatingIn = true;
  promptOverlayCurrentScale = 0.1f;
  promptOverlayTargetScale = 1.0f;
  promptOverlayAnimStartTime = millis();
}

void handleMenuSelection() {
  MenuState originalMenu = currentMenu;
  int newMenuIndexForSubMenu = 0;
  bool needsReinit = true;  // Default to true, set to false if no re-init is needed

  if (currentMenu == MAIN_MENU) {
    mainMenuSavedIndex = menuIndex;
    MenuState targetMenu = MAIN_MENU;  // Default to current menu
    if (menuIndex >= 0 && menuIndex < getMainMenuItemsCount()) {
      const char* selectedItem = mainMenuItems[menuIndex];
      if (strcmp(selectedItem, "Tools") == 0) targetMenu = TOOLS_MENU;
      else if (strcmp(selectedItem, "Games") == 0) targetMenu = GAMES_MENU;
      else if (strcmp(selectedItem, "Settings") == 0) targetMenu = SETTINGS_MENU;
      else if (strcmp(selectedItem, "Utilities") == 0) targetMenu = UTILITIES_MENU;
      else if (strcmp(selectedItem, "Info") == 0) {
        Serial.println("Info selected (NYI)");
        needsReinit = false;  // No menu change, no re-init
      }
    }
    if (targetMenu != currentMenu) {
      currentMenu = targetMenu;
      menuIndex = newMenuIndexForSubMenu;
      // needsReinit is already true
    } else if (targetMenu == currentMenu && needsReinit) {
      // If target menu is same but needsReinit was true (e.g. for "Info" if it did something)
      // This case is unlikely for MAIN_MENU selections that don't change menu.
    } else {
      needsReinit = false;  // No change occurred
    }

  } else if (currentMenu == TOOLS_MENU) {
    if (menuIndex == getToolsMenuItemsCount() - 1) {  // "Back" selected
      currentMenu = MAIN_MENU;
      menuIndex = mainMenuSavedIndex;
    } else {
      toolsCategoryIndex = menuIndex;
      currentMenu = TOOL_CATEGORY_GRID;
      menuIndex = newMenuIndexForSubMenu;
    }
    // needsReinit is already true

  } else if (currentMenu == GAMES_MENU) {
    if (menuIndex == getGamesMenuItemsCount() - 1) {  // "Back" selected
      currentMenu = MAIN_MENU;
      menuIndex = mainMenuSavedIndex;
      // needsReinit is already true
    } else {
      Serial.printf("Game: %s (NYI)\n", gamesMenuItems[menuIndex]);
      needsReinit = false;  // No menu change if game not implemented
    }

  } else if (currentMenu == SETTINGS_MENU) {
    if (menuIndex == getSettingsMenuItemsCount() - 1) {  // "Back" selected
      currentMenu = MAIN_MENU;
      menuIndex = mainMenuSavedIndex;
      // needsReinit is already true
    } else if (strcmp(settingsMenuItems[menuIndex], "Wi-Fi Setup") == 0) {
      currentMenu = WIFI_SETUP_MENU;
      wifiMenuIndex = 0;
      targetWifiListScrollOffset_Y = 0;
      currentWifiListScrollOffset_Y_anim = 0;

      if (!wifiHardwareEnabled) {
        Serial.println("handleMenuSelection (Wi-Fi Setup): Enabling Wi-Fi...");
        setWifiHardwareState(true, RF_MODE_NORMAL_STA);
        ;
        delay(300);
      } else {
        delay(100);
      }

      if (wifiHardwareEnabled) {
        // Set Kiva's state to "intending to scan" BEFORE calling initializeCurrentMenu
        wifiIsScanning = true;
        foundWifiNetworksCount = 0;

        initializeCurrentMenu();  // This will now correctly set up UI for "Scanning..."
                                  // because wifiIsScanning is true.

        Serial.println("handleMenuSelection (Wi-Fi Setup): Attempting scan.");
        int scanReturn = initiateAsyncWifiScan();  // This sets internal wifiStatusString

        if (scanReturn == WIFI_SCAN_FAILED || (scanReturn != WIFI_SCAN_RUNNING && scanReturn != -1)) {
          Serial.println("handleMenuSelection (Wi-Fi Setup): Initial scan attempt failed. Retrying once...");
          delay(250);  // Slightly longer delay for retry
          scanReturn = initiateAsyncWifiScan();
          if (scanReturn == WIFI_SCAN_FAILED || (scanReturn != WIFI_SCAN_RUNNING && scanReturn != -1)) {
            Serial.println("handleMenuSelection (Wi-Fi Setup): Scan retry also failed.");
            wifiIsScanning = false;
            checkAndRetrieveWifiScanResults();  // This will update wifiStatusString to "Scan Failed"
            initializeCurrentMenu();            // Re-init to show the "Scan Failed" state correctly
          } else if (scanReturn == WIFI_SCAN_RUNNING || scanReturn == -1) {
            Serial.println("handleMenuSelection (Wi-Fi Setup): Scan retry initiated successfully (async).");
            lastWifiScanCheckTime = millis();
            // wifiIsScanning is already true, initializeCurrentMenu was called.
          } else {
            Serial.println("handleMenuSelection (Wi-Fi Setup): Scan retry succeeded synchronously.");
            wifiIsScanning = false;
            checkAndRetrieveWifiScanResults();
            initializeCurrentMenu();  // Re-init to show scan results
          }
        } else if (scanReturn == WIFI_SCAN_RUNNING || scanReturn == -1) {
          Serial.println("handleMenuSelection (Wi-Fi Setup): Scan initiated successfully (async).");
          lastWifiScanCheckTime = millis();
          // wifiIsScanning is true, initializeCurrentMenu was called.
        } else {
          Serial.println("handleMenuSelection (Wi-Fi Setup): Scan succeeded synchronously on first try.");
          wifiIsScanning = false;
          checkAndRetrieveWifiScanResults();
          initializeCurrentMenu();  // Re-init to show scan results
        }
      } else {
        wifiIsScanning = false;
        foundWifiNetworksCount = 0;
        initializeCurrentMenu();  // Will show "Wi-Fi Off"
      }
      // needsReinit will be handled by the outer main function based on menu change.
      // Here, currentMenu definitely changed to WIFI_SETUP_MENU.
    } else if (strcmp(settingsMenuItems[menuIndex], "Firmware Update") == 0) {
      if (isJammingOperationActive) {
        otaStatusMessage = "Disable Jammer!";
        Serial.println("MenuLogic: " + otaStatusMessage);
        needsReinit = false;  // Don't change menu, stay on settings to show error (if UI supports it)
      } else {
        resetOtaState();
        currentMenu = FIRMWARE_UPDATE_GRID;
        menuIndex = newMenuIndexForSubMenu;
        // needsReinit is true
      }
    } else {
      Serial.printf("Setting: %s (NYI)\n", settingsMenuItems[menuIndex]);
      needsReinit = false;
    }
  } else if (currentMenu == FIRMWARE_UPDATE_GRID) {
    const char* selectedOtaItem = otaMenuItems[menuIndex];
    MenuState nextMenuTarget = currentMenu;
    bool otaStartAttempted = false;

    if (strcmp(selectedOtaItem, "Update via Web") == 0) {
      otaStartAttempted = true;
      if (startWebOtaUpdate()) {
        nextMenuTarget = OTA_WEB_ACTIVE;
      }
    } else if (strcmp(selectedOtaItem, "Update from SD") == 0) {
      otaStartAttempted = true;
      nextMenuTarget = FIRMWARE_SD_LIST_MENU;
      wifiMenuIndex = newMenuIndexForSubMenu;
    } else if (strcmp(selectedOtaItem, "Basic OTA (IDE)") == 0) {
      otaStartAttempted = true;
      if (startBasicOtaUpdate()) {
        nextMenuTarget = OTA_BASIC_ACTIVE;
      } else {
        if (!showPromptOverlay) {
          nextMenuTarget = OTA_BASIC_ACTIVE;
        }
      }
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
      // needsReinit is true
    } else if (otaStartAttempted && !showPromptOverlay) {
      // needsReinit is true
    } else if (!otaStartAttempted) {  // e.g. if selection was for an item that didn't change menu or attempt OTA
      needsReinit = false;
    }

  } else if (currentMenu == FIRMWARE_SD_LIST_MENU) {
    if (availableSdFirmwareCount > 0 && wifiMenuIndex < availableSdFirmwareCount) {
      g_firmwareToInstallFromSd = availableSdFirmwares[wifiMenuIndex];
      resetOtaState();
      otaStatusMessage = "Preparing: " + String(g_firmwareToInstallFromSd.version).substring(0, 15);
      otaProgress = OTA_PROGRESS_PENDING_SD_START;
      currentMenu = OTA_SD_STATUS;
    } else {
      currentMenu = FIRMWARE_UPDATE_GRID;
      for (int i = 0; i < getOtaMenuItemsCount(); ++i) {
        if (strcmp(otaMenuItems[i], "Update from SD") == 0) {
          menuIndex = i;
          break;
        }
      }
    }
    // needsReinit is true
  } else if (currentMenu == TOOL_CATEGORY_GRID) {
    int currentMaxItemsInGrid = 0;
    bool isJammingCat = false;
    const char** currentToolItemsListPtr = nullptr;

    if (toolsCategoryIndex >= 0 && toolsCategoryIndex < getToolsMenuItemsCount()) {
      if (strcmp(toolsMenuItems[toolsCategoryIndex], "Jamming") == 0) {
        isJammingCat = true;
        currentMaxItemsInGrid = getJammingToolItemsCount();
        currentToolItemsListPtr = jammingToolItems;
      } else {
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
      // needsReinit is true
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
      if (jamTypeToStart != JAM_NONE) {
        if (startActiveJamming(jamTypeToStart)) {
          currentMenu = JAMMING_ACTIVE_SCREEN;
          // needsReinit is true
        } else {
          Serial.println("Failed to start jamming");
          needsReinit = false;
        }
      } else {
        needsReinit = false;  // No valid jam type selected
      }
    } else if (!isJammingCat && currentMaxItemsInGrid > 0 && menuIndex >= 0 && menuIndex < (currentMaxItemsInGrid - 1)) {
      const char* itemName = (currentToolItemsListPtr && menuIndex < currentMaxItemsInGrid) ? currentToolItemsListPtr[menuIndex] : "Unknown Item";

      if (strcmp(itemName, "Beacon Spam") == 0) {
        start_beacon_spam(false, 1);  // Start Beacon Spam (not Rick Roll) on channel 1
        if (isBeaconSpamActive) {
          currentMenu = WIFI_BEACON_SPAM_ACTIVE_SCREEN;
          initializeCurrentMenu();  // Prepare the new screen's state
          drawUI();                 // FORCE a draw call HERE to show the "Active" screen
        }
        needsReinit = false;  // We handled the state change and drawing manually
      } else if (strcmp(itemName, "Rick Roll Spam") == 0) {
        start_beacon_spam(true, 1);  // Start Rick Roll on channel 1
        if (isRickRollActive) {
          currentMenu = WIFI_RICK_ROLL_ACTIVE_SCREEN;
          initializeCurrentMenu();  // Prepare the new screen's state
          drawUI();                 // FORCE a draw call HERE
        }
        needsReinit = false;  // We handled the state change and drawing manually
      } else {
        const char* catName = (toolsCategoryIndex >= 0 && toolsCategoryIndex < getToolsMenuItemsCount()) ? toolsMenuItems[toolsCategoryIndex] : "Unknown Cat";
        Serial.printf("Tool '%s' from cat '%s' (NYI)\n", itemName, catName);
        needsReinit = false;
      }
    } else {
      needsReinit = false;  // e.g. invalid index if grid is empty
    }
  } else if (currentMenu == WIFI_BEACON_SPAM_ACTIVE_SCREEN) {
    // 0: Start/Stop, 1: Back
    if (menuIndex == 0) {  // Start/Stop selected
      if (isBeaconSpamActive) {
        stop_beacon_spam();
      } else {
        start_beacon_spam(false, 1);  // Start on channel 1 by default
      }
      needsReinit = false;         // No menu change, but UI will update based on isBeaconSpamActive flag
    } else if (menuIndex == 1) {   // Back selected
      handleMenuBackNavigation();  // Use the existing back navigation logic
      needsReinit = false;         // Back navigation already handles re-init
    }
  } else if (currentMenu == WIFI_SETUP_MENU) {
    MenuState nextWiFiMenu = currentMenu;
    bool wifiMenuNeedsReinitForScan = false;

    if (!wifiHardwareEnabled) {
      if (maxMenuItems > 0 && wifiMenuIndex == (maxMenuItems - 1)) {
        nextWiFiMenu = SETTINGS_MENU;
        for (int i = 0; i < getSettingsMenuItemsCount(); ++i) {
          if (strcmp(settingsMenuItems[i], "Wi-Fi Setup") == 0) {
            menuIndex = i;
            break;
          }
        }
      } else if (maxMenuItems > 0 && wifiMenuIndex == 0) {
        setWifiHardwareState(true, RF_MODE_NORMAL_STA);
        ;
        delay(300);

        if (wifiHardwareEnabled) {
          wifiIsScanning = true;
          foundWifiNetworksCount = 0;
          wifiMenuNeedsReinitForScan = true;

          Serial.println("handleMenuSelection (Enable Wi-Fi): Attempting scan.");
          int scanReturn = initiateAsyncWifiScan();
          if (scanReturn == WIFI_SCAN_FAILED || (scanReturn != WIFI_SCAN_RUNNING && scanReturn != -1)) {
            Serial.println("handleMenuSelection (Enable Wi-Fi): Initial scan attempt failed. Retrying once...");
            delay(200);
            scanReturn = initiateAsyncWifiScan();
            if (scanReturn == WIFI_SCAN_FAILED || (scanReturn != WIFI_SCAN_RUNNING && scanReturn != -1)) {
              Serial.println("handleMenuSelection (Enable Wi-Fi): Scan retry also failed.");
              wifiIsScanning = false;
              checkAndRetrieveWifiScanResults();
            } else if (scanReturn == WIFI_SCAN_RUNNING || scanReturn == -1) {
              Serial.println("handleMenuSelection (Enable Wi-Fi): Scan retry initiated successfully (async).");
              lastWifiScanCheckTime = millis();
            } else {
              Serial.println("handleMenuSelection (Enable Wi-Fi): Scan retry succeeded synchronously.");
              wifiIsScanning = false;
              checkAndRetrieveWifiScanResults();
            }
          } else if (scanReturn == WIFI_SCAN_RUNNING || scanReturn == -1) {
            Serial.println("handleMenuSelection (Enable Wi-Fi): Scan initiated successfully (async).");
            lastWifiScanCheckTime = millis();
          } else {
            Serial.println("handleMenuSelection (Enable Wi-Fi): Scan succeeded synchronously on first try.");
            wifiIsScanning = false;
            checkAndRetrieveWifiScanResults();
          }
        } else {
          wifiMenuNeedsReinitForScan = true;
        }
      }
    } else if (!wifiIsScanning) {
      if (wifiMenuIndex < foundWifiNetworksCount) {
        if (currentConnectedSsid.length() > 0 && strcmp(scannedNetworks[wifiMenuIndex].ssid, currentConnectedSsid.c_str()) == 0) {
          activatePromptOverlay("Disconnect WiFi",
                                ("Disconnect from " + currentConnectedSsid + "?").c_str(),
                                "X", "O",
                                WIFI_SETUP_MENU,
                                []() {
                                  Serial.printf("Prompt: Disconnecting from %s...\n", currentConnectedSsid.c_str());
                                  WiFi.disconnect(true);
                                  updateWifiStatusOnDisconnect();
                                  initializeCurrentMenu();
                                });
          needsReinit = false;  // Prompt is active, main loop will handle UI update
          return;
        } else {
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
            addOrUpdateKnownNetwork(currentSsidToConnect, "", true);
            nextWiFiMenu = WIFI_CONNECTING;
          }
        }
      } else if (wifiMenuIndex == foundWifiNetworksCount) {
        wifiIsScanning = true;
        foundWifiNetworksCount = 0;
        wifiMenuNeedsReinitForScan = true;

        Serial.println("handleMenuSelection (Scan Again): Attempting scan.");
        int scanReturn = initiateAsyncWifiScan();
        if (scanReturn == WIFI_SCAN_FAILED || (scanReturn != WIFI_SCAN_RUNNING && scanReturn != -1)) {
          Serial.println("handleMenuSelection (Scan Again): Initial scan attempt failed. Retrying once...");
          delay(200);
          scanReturn = initiateAsyncWifiScan();
          if (scanReturn == WIFI_SCAN_FAILED || (scanReturn != WIFI_SCAN_RUNNING && scanReturn != -1)) {
            Serial.println("handleMenuSelection (Scan Again): Scan retry also failed.");
            wifiIsScanning = false;
            checkAndRetrieveWifiScanResults();
          } else if (scanReturn == WIFI_SCAN_RUNNING || scanReturn == -1) {
            Serial.println("handleMenuSelection (Scan Again): Scan retry initiated successfully (async).");
            lastWifiScanCheckTime = millis();
          } else {
            Serial.println("handleMenuSelection (Scan Again): Scan retry succeeded synchronously.");
            wifiIsScanning = false;
            checkAndRetrieveWifiScanResults();
          }
        } else if (scanReturn == WIFI_SCAN_RUNNING || scanReturn == -1) {
          Serial.println("handleMenuSelection (Scan Again): Scan initiated successfully (async).");
          lastWifiScanCheckTime = millis();
        } else {
          Serial.println("handleMenuSelection (Scan Again): Scan succeeded synchronously on first try.");
          wifiIsScanning = false;
          checkAndRetrieveWifiScanResults();
        }
      } else if (wifiMenuIndex == foundWifiNetworksCount + 1) {
        nextWiFiMenu = SETTINGS_MENU;
        for (int i = 0; i < getSettingsMenuItemsCount(); ++i) {
          if (strcmp(settingsMenuItems[i], "Wi-Fi Setup") == 0) {
            menuIndex = i;
            break;
          }
        }
      }
    }

    if (nextWiFiMenu != currentMenu) {
      currentMenu = nextWiFiMenu;
      // needsReinit is already true
    } else if (wifiMenuNeedsReinitForScan) {
      // needsReinit is already true
    } else {
      needsReinit = false;  // No change or scan initiation
    }
  } else if (currentMenu == UTILITIES_MENU) {
    const char* selectedItem = utilitiesMenuItems[menuIndex];
    if (strcmp(selectedItem, "Vibration") == 0) {
      vibrationOn = !vibrationOn;
      setOutputOnPCF0(MOTOR_PIN_PCF0, vibrationOn);
      needsReinit = false;  // UI for this item updates dynamically within drawCarouselMenu
    } else if (strcmp(selectedItem, "Laser") == 0) {
      laserOn = !laserOn;
      setOutputOnPCF0(LASER_PIN_PCF0, laserOn);
      needsReinit = false;  // UI for this item updates dynamically
    } else if (strcmp(selectedItem, "Flashlight") == 0) {
      currentMenu = FLASHLIGHT_MODE;
      // needsReinit is true
    } else if (strcmp(selectedItem, "Back") == 0) {
      currentMenu = MAIN_MENU;
      for (int i = 0; i < getMainMenuItemsCount(); ++i) {
        if (strcmp(mainMenuItems[i], "Utilities") == 0) {
          menuIndex = i;
          mainMenuSavedIndex = i;
          break;
        }
      }
      // needsReinit is true
    }
  } else if (currentMenu == FLASHLIGHT_MODE) {
    currentMenu = UTILITIES_MENU;
    for (int i = 0; i < getUtilitiesMenuItemsCount(); ++i) {
      if (strcmp(utilitiesMenuItems[i], "Flashlight") == 0) {
        menuIndex = i;
        break;
      }
    }
    // needsReinit is true
  } else {
    needsReinit = false;  // Default: no re-init if menu not handled above
  }

  if (needsReinit) {
    initializeCurrentMenu();
  }
}

void handleMenuBackNavigation() {
  MenuState previousMenuState = currentMenu;
  bool menuChanged = true;

  if (currentMenu == OTA_WEB_ACTIVE) {
    stopWebOtaUpdate();
    currentMenu = FIRMWARE_UPDATE_GRID;
    menuIndex = 0;  // Select "Update via Web"
  } else if (currentMenu == OTA_SD_STATUS) {
    resetOtaState();
    currentMenu = FIRMWARE_SD_LIST_MENU;
    wifiMenuIndex = 0;  // Select first firmware or "Back"
  } else if (currentMenu == OTA_BASIC_ACTIVE) {
    stopBasicOtaUpdate();
    currentMenu = FIRMWARE_UPDATE_GRID;
    menuIndex = 2;  // Select "Basic OTA (IDE)"
  } else if (currentMenu == FIRMWARE_SD_LIST_MENU) {
    resetOtaState();
    currentMenu = FIRMWARE_UPDATE_GRID;
    for (int i = 0; i < getOtaMenuItemsCount(); ++i) {
      if (strcmp(otaMenuItems[i], "Update from SD") == 0) {
        menuIndex = i;
        break;
      }
    }
  } else if (currentMenu == FIRMWARE_UPDATE_GRID) {
    resetOtaState();
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
    if (!foundJammingCat) toolsCategoryIndex = 4;  // Fallback, assuming Jamming is usually index 4
    menuIndex = lastSelectedJammingToolGridIndex;
    int numJammingItems = getJammingToolItemsCount();
    if (numJammingItems > 0) menuIndex = constrain(menuIndex, 0, numJammingItems - 1);
    else menuIndex = 0;
  } else if (currentMenu == WIFI_BEACON_SPAM_ACTIVE_SCREEN || currentMenu == WIFI_RICK_ROLL_ACTIVE_SCREEN) {  // <--- ADD THIS BLOCK
    stop_beacon_spam();
    currentMenu = TOOL_CATEGORY_GRID;
    // Find "Wi-Fi Attacks" category to restore grid state
    for (int i = 0; i < getToolsMenuItemsCount(); ++i) {
      if (strcmp(toolsMenuItems[i], "Wi-Fi Attacks") == 0) {
        toolsCategoryIndex = i;
        break;
      }
    }
    // Find "Beacon Spam" item to restore selected index
    int wifiAttackItemsCount = getWifiAttackToolItemsCount();
    for (int i = 0; i < wifiAttackItemsCount; i++) {
      if (strcmp(wifiAttackToolItems[i], "Beacon Spam") == 0) {
        menuIndex = i;
        break;
      }
    }
  } else if (currentMenu == MAIN_MENU) {
    menuChanged = false;  // Cannot go back from main menu
  } else if (currentMenu == WIFI_PASSWORD_INPUT) {
    currentMenu = WIFI_SETUP_MENU;
    wifiPasswordInput[0] = '\0';  // Clear password
    wifiPasswordInputCursor = 0;
    // wifiMenuIndex in WIFI_SETUP_MENU should ideally point to the network previously selected.
    // This requires storing that index before going to password input. For now, it might reset to 0.
    // Or, find the currentSsidToConnect in the scannedNetworks to set wifiMenuIndex.
    bool foundNetwork = false;
    for (int i = 0; i < foundWifiNetworksCount; ++i) {
      if (strcmp(scannedNetworks[i].ssid, currentSsidToConnect) == 0) {
        wifiMenuIndex = i;
        foundNetwork = true;
        break;
      }
    }
    if (!foundNetwork) wifiMenuIndex = 0;  // Default to first item if not found

  } else if (currentMenu == WIFI_CONNECTING || currentMenu == WIFI_CONNECTION_INFO) {
    if (WiFi.status() != WL_NO_SHIELD && WiFi.status() != WL_IDLE_STATUS) {
      WiFi.disconnect(true);  // Disconnect if attempting or connected
      delay(100);
      updateWifiStatusOnDisconnect();  // Update internal status
    }
    currentMenu = WIFI_SETUP_MENU;
    wifiMenuIndex = 0;  // Reset list position
    targetWifiListScrollOffset_Y = 0;
    currentWifiListScrollOffset_Y_anim = 0;
    // Optionally: re-scan or ensure the list is fresh upon returning
    // For now, it will show the last scanned list.
  } else if (currentMenu == TOOL_CATEGORY_GRID) {
    currentMenu = TOOLS_MENU;
    menuIndex = toolsCategoryIndex;                         // Select the category in the parent TOOLS_MENU
    if (isJammingOperationActive) { stopActiveJamming(); }  // Safety stop
  } else if (currentMenu == WIFI_SETUP_MENU) {
    if (wifiIsScanning) {
      wifiIsScanning = false;
      WiFi.scanDelete();  // Stop any ongoing scan
    }
    if (wifiConnectForScheduledAction) {
      currentMenu = postWifiActionMenu;  // Go to where the action was scheduled from
      // Example: If it was OTA_BASIC_ACTIVE, menuIndex for that screen might need setting
      if (postWifiActionMenu == FIRMWARE_UPDATE_GRID) {
        for (int i = 0; i < getOtaMenuItemsCount(); ++i) {
          if (strcmp(otaMenuItems[i], "Basic OTA (IDE)") == 0) {  // Assume it was for basic ota
            menuIndex = i;
            break;
          }
        }
      }
      wifiConnectForScheduledAction = false;
    } else {  // Normal back from Wi-Fi setup
      currentMenu = SETTINGS_MENU;
      for (int i = 0; i < getSettingsMenuItemsCount(); ++i) {
        if (strcmp(settingsMenuItems[i], "Wi-Fi Setup") == 0) {
          menuIndex = i;
          break;
        }
      }
    }
    // Auto-disable Wi-Fi if not connected and no longer in Wi-Fi menus (handled by main loop)
  } else if (currentMenu == UTILITIES_MENU) {
    currentMenu = MAIN_MENU;
    for (int i = 0; i < getMainMenuItemsCount(); ++i) {
      if (strcmp(mainMenuItems[i], "Utilities") == 0) {
        menuIndex = i;
        mainMenuSavedIndex = i;
        break;
      }
    }
  } else if (currentMenu == FLASHLIGHT_MODE) {
    currentMenu = UTILITIES_MENU;
    for (int i = 0; i < getUtilitiesMenuItemsCount(); ++i) {
      if (strcmp(utilitiesMenuItems[i], "Flashlight") == 0) {
        menuIndex = i;
        break;
      }
    }
  } else if (currentMenu == GAMES_MENU || currentMenu == TOOLS_MENU || currentMenu == SETTINGS_MENU || currentMenu == UTILITIES_MENU) {  // Modified this slightly
    currentMenu = MAIN_MENU;
    // Find the correct index in mainMenuItems based on previousMenuState
    const char* targetMainMenuItem = nullptr;
    if (previousMenuState == GAMES_MENU) targetMainMenuItem = "Games";
    else if (previousMenuState == TOOLS_MENU) targetMainMenuItem = "Tools";
    else if (previousMenuState == SETTINGS_MENU) targetMainMenuItem = "Settings";
    else if (previousMenuState == UTILITIES_MENU) targetMainMenuItem = "Utilities";

    if (targetMainMenuItem) {
      for (int i = 0; i < getMainMenuItemsCount(); ++i) {
        if (strcmp(mainMenuItems[i], targetMainMenuItem) == 0) {
          mainMenuSavedIndex = i;
          break;
        }
      }
    } else {  // Fallback if previousMenuState was not one of these
      mainMenuSavedIndex = 0;
    }
    menuIndex = mainMenuSavedIndex;
  } else {
    menuChanged = false;
  }

  if (menuChanged) {
    initializeCurrentMenu();
  }
}