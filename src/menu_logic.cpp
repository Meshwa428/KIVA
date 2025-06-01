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
  // gridAnimatingIn = false; // This should be set by startGridItemAnimation or when grid is left

  // Reset keyboard focus for any menu that isn't password input
  if (currentMenu != WIFI_PASSWORD_INPUT) {
    keyboardFocusRow = 0;
    keyboardFocusCol = 0;
    // currentKeyboardLayer = KB_LAYER_LOWERCASE; // Don't reset layer if just navigating menus
    // capsLockActive = false; // Don't reset caps lock on general menu init
  }

  switch (currentMenu) {
    case MAIN_MENU:
      maxMenuItems = getMainMenuItemsCount();
      if (menuIndex >= maxMenuItems) menuIndex = maxMenuItems > 0 ? maxMenuItems -1 : 0;
      if (menuIndex < 0) menuIndex = 0;
      mainMenuAnim.init();
      mainMenuAnim.setTargets(menuIndex, maxMenuItems);
      gridAnimatingIn = false; // Ensure grid animation flag is off
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
      
      targetGridScrollOffset_Y = 0; // Reset scroll on entering grid or changing category
      currentGridScrollOffset_Y_anim = 0;
      startGridItemAnimation(); // This sets gridAnimatingIn = true
      break;
    case WIFI_SETUP_MENU:
      if (!wifiHardwareEnabled) { 
        // If Wi-Fi is off, maybe show 1 item: "Enable Wi-Fi" and then "Back"
        // For simplicity, let's assume UI shows "Wi-Fi Off". Max items can be 1 for "Back".
        // Or 0 if "Back" is handled by physical button only.
        // If we want an "Enable Wi-Fi" item, need to adjust menu item text source.
        // For now, drawWifiSetupScreen handles the "Wi-Fi is Off" message.
        // Let's assume only "Back" is navigable if Wi-Fi is off for the list structure.
        // Or, make maxMenuItems = 0 and rely on `drawWifiSetupScreen` for the message.
        // Let's make it 1 for "Back" to be consistent with other empty lists.
        // maxMenuItems = 1; // "Back"
        // For now, let's be consistent with previous logic where drawWifiSetupScreen shows message.
        // If `wifiHardwareEnabled` is false, `foundWifiNetworksCount` will be 0.
        // `maxMenuItems` will then be 0 + 2 = 2 items ("Scan Again", "Back").
        // The UI function `drawWifiSetupScreen` will display "Wi-Fi is Off" instead of the list.
        // So, the existing logic for maxMenuItems should work with UI changes.
         maxMenuItems = foundWifiNetworksCount + 2; // "Scan Again", "Back"
         if (wifiIsScanning && wifiHardwareEnabled) maxMenuItems = 0; // "Scanning..."
      } else if (wifiIsScanning) {
        maxMenuItems = 0;  // "Scanning..." message takes over UI
      } else {
        maxMenuItems = foundWifiNetworksCount + 2;  // Networks + Scan Again + Back
      }

      wifiListAnim.init();
      if (maxMenuItems > 0) { // Check before accessing/setting wifiMenuIndex
        if (wifiMenuIndex >= maxMenuItems) wifiMenuIndex = maxMenuItems - 1;
        if (wifiMenuIndex < 0) wifiMenuIndex = 0;
      } else {
        wifiMenuIndex = 0; // Default if no items
      }
      wifiListAnim.setTargets(wifiMenuIndex, maxMenuItems);
      targetWifiListScrollOffset_Y = 0;  // Reset scroll on re-entry or scan finish
      currentWifiListScrollOffset_Y_anim = 0;
      gridAnimatingIn = false;
      break;
    case FLASHLIGHT_MODE:
      maxMenuItems = 0;  // No selectable items in flashlight mode
      gridAnimatingIn = false;
      break;
    case WIFI_PASSWORD_INPUT:
      maxMenuItems = 0;  // No list items, UI is custom for password input
      // Keyboard state init
      keyboardFocusRow = 0;
      keyboardFocusCol = 0;
      currentKeyboardLayer = KB_LAYER_LOWERCASE; // Default to lowercase
      capsLockActive = false;
      gridAnimatingIn = false;
      // Password buffer (wifiPasswordInput) is expected to be initialized by caller or persist
      break;
    case WIFI_CONNECTING:
    case WIFI_CONNECTION_INFO:
      maxMenuItems = 0;  // Custom UI, no list items
      gridAnimatingIn = false;
      break;
  }

  // General bounds check for menuIndex if not handled by specific cases above
  // (WIFI_SETUP_MENU uses wifiMenuIndex, handled in its case)
  if (currentMenu != WIFI_SETUP_MENU && currentMenu != WIFI_PASSWORD_INPUT && 
      currentMenu != WIFI_CONNECTING && currentMenu != WIFI_CONNECTION_INFO) {
    if (maxMenuItems > 0) {
      if (menuIndex >= maxMenuItems) menuIndex = maxMenuItems - 1;
      if (menuIndex < 0) menuIndex = 0;
    } else if (menuIndex != 0) { // If maxMenuItems is 0, index must be 0
      menuIndex = 0;
    }
  }
}


void handleMenuSelection() {
  MenuState previousMenu = currentMenu;
  int localMenuIndexForNewMenu = 0; // Used to reset index when entering a new sub-menu

  if (currentMenu == MAIN_MENU) {
    mainMenuSavedIndex = menuIndex; // Save current main menu selection for returning
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
      // Handle Info screen selection - for now, let's assume it's a placeholder
      Serial.println("Info selected - (Not yet implemented)");
      return; 
    }

    if (targetMenu != currentMenu) {
      currentMenu = targetMenu;
      menuIndex = localMenuIndexForNewMenu; // Reset index for the new menu
      initializeCurrentMenu();
    }
  } else if (currentMenu == TOOLS_MENU) {
    int currentMax = getToolsMenuItemsCount();
    if (menuIndex == (currentMax - 1)) {  // "Back" selected from Tools menu
      currentMenu = MAIN_MENU;
      // Try to restore previous main menu selection for "Tools"
      for (int i = 0; i < getMainMenuItemsCount(); ++i) {
        if (strcmp(mainMenuItems[i], "Tools") == 0) {
          menuIndex = i;
          mainMenuSavedIndex = i; // Ensure mainMenuSavedIndex is also updated
          break;
        }
      }
    } else { // A tool category selected
      toolsCategoryIndex = menuIndex; // Store which category was selected
      currentMenu = TOOL_CATEGORY_GRID;
      menuIndex = localMenuIndexForNewMenu; // Reset index for the grid
    }
    initializeCurrentMenu(); // Initialize the new menu (MAIN_MENU or TOOL_CATEGORY_GRID)
  } else if (currentMenu == GAMES_MENU) {
    int currentMax = getGamesMenuItemsCount();
    if (menuIndex == (currentMax - 1)) {  // "Back" selected from Games menu
      currentMenu = MAIN_MENU;
      for (int i = 0; i < getMainMenuItemsCount(); ++i) {
        if (strcmp(mainMenuItems[i], "Games") == 0) {
          menuIndex = i;
          mainMenuSavedIndex = i;
          break;
        }
      }
    } else { 
      // Game launch logic would go here
      Serial.printf("Selected game: %s (Not yet implemented)\n", gamesMenuItems[menuIndex]);
    }
    // Only initialize if the menu actually changed (e.g., went back to MAIN_MENU)
    // If a game was "launched" (but not yet implemented), currentMenu might not change.
    if (previousMenu != currentMenu) {
        initializeCurrentMenu();
    }
  } else if (currentMenu == SETTINGS_MENU) {
    int currentMax = getSettingsMenuItemsCount();
    if (menuIndex == (currentMax - 1)) {  // "Back" selected from Settings menu
      currentMenu = MAIN_MENU;
      for (int i = 0; i < getMainMenuItemsCount(); ++i) {
        if (strcmp(mainMenuItems[i], "Settings") == 0) {
          menuIndex = i;
          mainMenuSavedIndex = i;
          break;
        }
      }
    } else if (strcmp(settingsMenuItems[menuIndex], "Wi-Fi Setup") == 0) {
      setWifiHardwareState(true); // Turn on Wi-Fi when entering setup
      delay(100); // Give a moment for hardware to initialize if it was off

      currentMenu = WIFI_SETUP_MENU;
      wifiMenuIndex = localMenuIndexForNewMenu; // Reset Wi-Fi list selection
      int scanReturn = initiateAsyncWifiScan(); // from wifi_manager
      
      if (scanReturn == WIFI_SCAN_RUNNING || scanReturn == -1) { // ESP32 uses -1 for WIFI_SCAN_RUNNING
        wifiIsScanning = true;
        foundWifiNetworksCount = 0; // Reset count while scanning
        lastWifiScanCheckTime = millis();
      } else if (scanReturn == -3) { // Custom code for Wi-Fi disabled (from initiateAsyncWifiScan)
        Serial.println("Wi-Fi is disabled by manager. Cannot scan.");
        wifiIsScanning = false;
        foundWifiNetworksCount = 0; 
        // initializeCurrentMenu will be called below and UI should reflect "Wi-Fi Off"
      } else { // Scan failed to start or completed synchronously (less common for async)
        wifiIsScanning = false;
        checkAndRetrieveWifiScanResults(); // Populate networks immediately
      }
    } else { 
      // Other settings logic
      Serial.printf("Selected setting: %s (Not yet implemented)\n", settingsMenuItems[menuIndex]);
    }
    if (previousMenu != currentMenu) { // Initialize if menu changed (e.g. to WIFI_SETUP_MENU or MAIN_MENU)
        initializeCurrentMenu();
    }
  } else if (currentMenu == TOOL_CATEGORY_GRID) {
    int currentMaxItemsInGrid = 0; // Determine max items for the current tool category
    switch (toolsCategoryIndex) {
      case 0: currentMaxItemsInGrid = getInjectionToolItemsCount(); break;
      case 1: currentMaxItemsInGrid = getWifiAttackToolItemsCount(); break;
      case 2: currentMaxItemsInGrid = getBleAttackToolItemsCount(); break;
      case 3: currentMaxItemsInGrid = getNrfReconToolItemsCount(); break;
      case 4: currentMaxItemsInGrid = getJammingToolItemsCount(); break;
      default: currentMaxItemsInGrid = 0; break;
    }

    if (menuIndex == currentMaxItemsInGrid - 1 && currentMaxItemsInGrid > 0) { // "Back" selected from tool grid
      currentMenu = TOOLS_MENU;
      menuIndex = toolsCategoryIndex; // Restore selection in Tools menu to the category we came from
    } else if (currentMaxItemsInGrid > 0) { 
      // Actual tool selected
      // Example: const char* selectedToolName = (*(getToolListPointer(toolsCategoryIndex)))[menuIndex];
      // Serial.printf("Selected tool: %s (Not yet implemented)\n", selectedToolName);
      Serial.println("Tool selected from grid (Not yet implemented)");
    }
    initializeCurrentMenu(); // Initialize new menu (TOOLS_MENU or refresh grid if tool selected - TBD)
  } else if (currentMenu == WIFI_SETUP_MENU) {
    if (!wifiHardwareEnabled) {
        // If Wi-Fi is off, the only "selectable" item might be "Back" or an "Enable Wi-Fi"
        // This logic assumes if Wi-Fi is off, item selection will be handled to enable it or go back.
        // For now, let's assume if we reach here and wifiMenuIndex points to "Back" (if it's the only option)
        if (maxMenuItems > 0 && wifiMenuIndex == maxMenuItems -1) { // Assuming "Back" is last
             currentMenu = SETTINGS_MENU;
             for (int i = 0; i < getSettingsMenuItemsCount(); ++i)
                if (strcmp(settingsMenuItems[i], "Wi-Fi Setup") == 0) {
                    menuIndex = i; break;
                }
             initializeCurrentMenu();
        } // Else, if another item like "Enable Wi-Fi" was selected, it would handle turning Wi-Fi on.
        return; // Don't process further if Wi-Fi is off.
    }

    if (!wifiIsScanning) { // Only handle selections if not actively scanning
      if (wifiMenuIndex < foundWifiNetworksCount) {  // A network SSID is selected
        strncpy(currentSsidToConnect, scannedNetworks[wifiMenuIndex].ssid, sizeof(currentSsidToConnect) - 1);
        currentSsidToConnect[sizeof(currentSsidToConnect) - 1] = '\0';
        selectedNetworkIsSecure = scannedNetworks[wifiMenuIndex].isSecure;

        KnownWifiNetwork* knownNet = findKnownNetwork(currentSsidToConnect);

        if (selectedNetworkIsSecure) {
            if (knownNet && knownNet->failCount < MAX_WIFI_FAIL_ATTEMPTS && strlen(knownNet->password) > 0) {
                // Found known secure network, fail count is acceptable, and has a password
                Serial.printf("Attempting connection to SECURE network %s using stored password.\n", currentSsidToConnect);
                attemptWpaWifiConnection(currentSsidToConnect, knownNet->password);
                currentMenu = WIFI_CONNECTING;
            } else {
                 // Not known, or fail count too high, or no password stored (should not happen for secure known net if logic is correct)
                 if (knownNet && knownNet->failCount >= MAX_WIFI_FAIL_ATTEMPTS) {
                    Serial.printf("Stored password for %s failed %d times. Asking for new password.\n", currentSsidToConnect, knownNet->failCount);
                 } else if (knownNet && strlen(knownNet->password) == 0 && selectedNetworkIsSecure) { // Should ideally not happen
                    Serial.printf("Known network %s is secure but has no password stored. Asking for password.\n", currentSsidToConnect);
                 } else if (!knownNet) {
                    Serial.printf("Network %s is not known. Asking for password.\n", currentSsidToConnect);
                 }
                currentMenu = WIFI_PASSWORD_INPUT;
                wifiPasswordInput[0] = '\0';  // Clear password buffer for new input
                wifiPasswordInputCursor = 0;
            }
        } else { // Open network
            Serial.printf("Attempting connection to OPEN network %s.\n", currentSsidToConnect);
            attemptDirectWifiConnection(currentSsidToConnect); // This will also add/update it as known with empty pass
            currentMenu = WIFI_CONNECTING;
        }
        initializeCurrentMenu(); // Initialize for WIFI_CONNECTING or WIFI_PASSWORD_INPUT state

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
        initializeCurrentMenu(); // Re-initialize menu with scanning status or new results
        wifiMenuIndex = 0; // Reset selection to the top of the new/updated list
        targetWifiListScrollOffset_Y = 0; // Reset scroll
        currentWifiListScrollOffset_Y_anim = 0;
        if (currentMenu == WIFI_SETUP_MENU) { // Ensure animation targets are set for the new list
             wifiListAnim.setTargets(wifiMenuIndex, maxMenuItems);
        }

      } else if (wifiMenuIndex == foundWifiNetworksCount + 1) {  // "Back" selected from Wi-Fi list
        currentMenu = SETTINGS_MENU;
        for (int i = 0; i < getSettingsMenuItemsCount(); ++i)
          if (strcmp(settingsMenuItems[i], "Wi-Fi Setup") == 0) {
            menuIndex = i; // Restore selection in settings menu
            break;
          }
        initializeCurrentMenu();
      }
    }
  } else if (currentMenu == UTILITIES_MENU) {
    const char* selectedItem = utilitiesMenuItems[menuIndex];
    bool menuChanged = false; // Flag to check if we need to re-initialize the menu
    if (strcmp(selectedItem, "Vibration") == 0) {
      vibrationOn = !vibrationOn;
      setOutputOnPCF0(MOTOR_PIN_PCF0, vibrationOn);
      // No menu change, just toggle state. UI for item text might update in drawCarouselMenu.
    } else if (strcmp(selectedItem, "Laser") == 0) {
      laserOn = !laserOn;
      setOutputOnPCF0(LASER_PIN_PCF0, laserOn);
      // No menu change.
    } else if (strcmp(selectedItem, "Flashlight") == 0) {
      currentMenu = FLASHLIGHT_MODE;
      menuChanged = true;
    } else if (strcmp(selectedItem, "Back") == 0) {
      currentMenu = MAIN_MENU;
      menuChanged = true;
      // Restore main menu selection for "Utilities"
      for (int i = 0; i < getMainMenuItemsCount(); ++i)
        if (strcmp(mainMenuItems[i], "Utilities") == 0) {
          menuIndex = i;
          mainMenuSavedIndex = i;
          break;
        }
    }
    if (menuChanged) { // Only re-initialize if menu itself changed (e.g. to Flashlight or Main)
        initializeCurrentMenu();
    }
  } else if (currentMenu == FLASHLIGHT_MODE) {
    // OK press in flashlight mode typically means go back
    currentMenu = UTILITIES_MENU;
    // Restore selection in Utilities menu to "Flashlight"
    for (int i = 0; i < getUtilitiesMenuItemsCount(); ++i)
      if (strcmp(utilitiesMenuItems[i], "Flashlight") == 0) {
        menuIndex = i;
        break;
      }
    initializeCurrentMenu();
  }
  // Note: WIFI_PASSWORD_INPUT selection (Enter key) is handled in input_handling.cpp's handleKeyboardInput.
  // WIFI_CONNECTING and WIFI_CONNECTION_INFO are transitional states; user "OK" presses in these
  // states are generally ignored as the system manages transitions based on connection status.
}

void handleMenuBackNavigation() {
  MenuState previousMenuState = currentMenu; // Store current state before changing

  if (currentMenu == MAIN_MENU) {
    return;  // Cannot go back from main menu
  } else if (currentMenu == WIFI_PASSWORD_INPUT) {
    currentMenu = WIFI_SETUP_MENU;
    wifiPasswordInput[0] = '\0';  // Clear password buffer
    wifiPasswordInputCursor = 0;
    // wifiMenuIndex should ideally point to the network that led to password input.
    // initializeCurrentMenu will handle setting up the list again.
  } else if (currentMenu == WIFI_CONNECTING || currentMenu == WIFI_CONNECTION_INFO) {
    WiFi.disconnect(); // Stop any ongoing connection attempt
    currentMenu = WIFI_SETUP_MENU;  // Go back to the Wi-Fi list
    // Reset wifiMenuIndex to top or last selected if that info is preserved
    wifiMenuIndex = 0; 
    targetWifiListScrollOffset_Y = 0;
    currentWifiListScrollOffset_Y_anim = 0;
  } else if (currentMenu == TOOL_CATEGORY_GRID) {
    currentMenu = TOOLS_MENU;
    menuIndex = toolsCategoryIndex; // Restore selection to the category in Tools menu
  } else if (currentMenu == WIFI_SETUP_MENU) {
    if (wifiIsScanning) {
      // WiFi.scanDelete(); // Optionally stop an ongoing scan if library supports immediate cancel
      wifiIsScanning = false; // Stop checking scan results in main loop
    }
    currentMenu = SETTINGS_MENU;
    // Restore selection in Settings menu to "Wi-Fi Setup"
    for (int i = 0; i < getSettingsMenuItemsCount(); ++i) {
      if (strcmp(settingsMenuItems[i], "Wi-Fi Setup") == 0) {
        menuIndex = i;
        break;
      }
    }
    // The main loop's auto-Wi-Fi-disable logic will handle turning off Wi-Fi if appropriate
    // or, if you want explicit turn-off here if not connected:
    // if (wifiHardwareEnabled && WiFi.status() != WL_CONNECTED) {
    //   setWifiHardwareState(false);
    // }
  } else if (currentMenu == UTILITIES_MENU) {
    currentMenu = MAIN_MENU;
    // Restore selection in Main menu to "Utilities"
    for (int i = 0; i < getMainMenuItemsCount(); ++i)
      if (strcmp(mainMenuItems[i], "Utilities") == 0) {
        menuIndex = i;
        mainMenuSavedIndex = i; // Ensure mainMenuSavedIndex is also updated
        break;
      }
  } else if (currentMenu == FLASHLIGHT_MODE) {
    currentMenu = UTILITIES_MENU;
    // Restore selection in Utilities menu to "Flashlight"
    for (int i = 0; i < getUtilitiesMenuItemsCount(); ++i)
      if (strcmp(utilitiesMenuItems[i], "Flashlight") == 0) {
        menuIndex = i;
        break;
      }
  } else if (currentMenu == GAMES_MENU || currentMenu == TOOLS_MENU || currentMenu == SETTINGS_MENU) {
    // Generic back navigation from these sub-menus to Main Menu
    currentMenu = MAIN_MENU;
    // Restore selection in Main Menu based on which sub-menu we came from
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
    menuIndex = mainMenuSavedIndex; // Apply the restored main menu index
  }

  if (previousMenuState != currentMenu) { // If the menu state actually changed
    initializeCurrentMenu();
  }

  // Auto-disable Wi-Fi check is now primarily in the main loop
  // to handle various scenarios more globally.
}