#include "config.h"
#include "input_handling.h"
#include "pcf_utils.h"
#include "wifi_manager.h"
#include "menu_logic.h"
#include "keyboard_layout.h"

// Static variables for debouncing and button states (unchanged)
static bool prevDbncHState0[8] = { 1, 1, 1, 1, 1, 1, 1, 1 }, lastRawHState0[8] = { 1, 1, 1, 1, 1, 1, 1, 1 };
static unsigned long lastDbncT0[8] = { 0 };
static bool prevDbncHState1[8] = { 1, 1, 1, 1, 1, 1, 1, 1 }, lastRawHState1[8] = { 1, 1, 1, 1, 1, 1, 1, 1 };
static unsigned long lastDbncT1[8] = { 0 };
static unsigned long btnHoldStartT1[8] = { 0 }, lastRepeatT1[8] = { 0 };
static bool isBtnHeld1[8] = { 0 };


// QuadratureEncoder methods (unchanged)
// ...
void QuadratureEncoder::init(int iA, int iB) {
  position = 0;
  lastState = (iB << 1) | iA;
  lastValidTime = millis();
  consecutiveValid = 0;
}
bool QuadratureEncoder::update(int cA, int cB) {
  int curSt = (cB << 1) | cA;
  unsigned long n = millis();
  if (curSt == lastState) return false;
  if ((n - lastValidTime) < minInterval && consecutiveValid == 0) {}
  bool vCW = (curSt == cwTable[lastState]);
  bool vCCW = (curSt == ccwTable[lastState]);
  if (vCW || vCCW) {
    consecutiveValid++;
    if (consecutiveValid >= requiredConsecutive) {
      if (vCW) position++;
      else position--;
      lastValidTime = n;
      consecutiveValid = 0;
      lastState = curSt;
      return true;
    }
  } else {
    consecutiveValid = 0;
  }
  lastState = curSt;
  return false;
}

// setupInputs (unchanged)
void setupInputs() {
  selectMux(0);
  uint8_t initialPcf0State = readPCF(PCF0_ADDR);
  encoder.init(!(initialPcf0State & (1 << ENC_A)), !(initialPcf0State & (1 << ENC_B)));
}

// handleKeyboardInput (unchanged)
// ...
void handleKeyboardInput(int keyPressedValue) {
    if (wifiPasswordInputCursor < PASSWORD_MAX_LEN && keyPressedValue > 0 && keyPressedValue < 127) { // Printable ASCII
        wifiPasswordInput[wifiPasswordInputCursor++] = (char)keyPressedValue;
        wifiPasswordInput[wifiPasswordInputCursor] = '\0';
    } else {
        switch (keyPressedValue) {
            case KB_KEY_BACKSPACE:
                if (wifiPasswordInputCursor > 0) {
                    wifiPasswordInput[--wifiPasswordInputCursor] = '\0';
                }
                break;
            case KB_KEY_SPACE:
                if (wifiPasswordInputCursor < PASSWORD_MAX_LEN) {
                    wifiPasswordInput[wifiPasswordInputCursor++] = ' ';
                    wifiPasswordInput[wifiPasswordInputCursor] = '\0';
                }
                break;
            case KB_KEY_ENTER:
                if (strlen(currentSsidToConnect) > 0) { 
                    if (selectedNetworkIsSecure) {
                        attemptWpaWifiConnection(currentSsidToConnect, wifiPasswordInput);
                        addOrUpdateKnownNetwork(currentSsidToConnect, wifiPasswordInput, true);
                    } else {
                        attemptDirectWifiConnection(currentSsidToConnect); 
                    }
                    currentMenu = WIFI_CONNECTING;
                    initializeCurrentMenu(); 
                }
                break;
            case KB_KEY_SHIFT:
                if (capsLockActive) {
                    if (currentKeyboardLayer == KB_LAYER_UPPERCASE) currentKeyboardLayer = KB_LAYER_LOWERCASE;
                } else {
                    if (currentKeyboardLayer == KB_LAYER_LOWERCASE) currentKeyboardLayer = KB_LAYER_UPPERCASE;
                    else if (currentKeyboardLayer == KB_LAYER_UPPERCASE) currentKeyboardLayer = KB_LAYER_LOWERCASE;
                }
                break;
            case KB_KEY_CAPSLOCK:
                capsLockActive = !capsLockActive;
                if (capsLockActive) currentKeyboardLayer = KB_LAYER_UPPERCASE;
                else currentKeyboardLayer = KB_LAYER_LOWERCASE;
                break;
            case KB_KEY_TO_NUM:
                currentKeyboardLayer = KB_LAYER_NUMBERS; capsLockActive = false;
                keyboardFocusRow = 0; keyboardFocusCol = 0;
                break;
            case KB_KEY_TO_SYM:
                currentKeyboardLayer = KB_LAYER_SYMBOLS; capsLockActive = false;
                keyboardFocusRow = 0; keyboardFocusCol = 0;
                break;
            case KB_KEY_TO_LOWER:
                currentKeyboardLayer = KB_LAYER_LOWERCASE; capsLockActive = false;
                keyboardFocusRow = 0; keyboardFocusCol = 0;
                break;
            case KB_KEY_TO_UPPER:
                currentKeyboardLayer = KB_LAYER_UPPERCASE;
                keyboardFocusRow = 0; keyboardFocusCol = 0;
                break;
        }
    }
}


void updateInputs() {
    unsigned long curT = millis();
    selectMux(0);
    uint8_t pcf0S = readPCF(PCF0_ADDR);
    int encA_val = !(pcf0S & (1 << ENC_A));
    int encB_val = !(pcf0S & (1 << ENC_B));
    static int lastEncoderPosition = encoder.position;

    // --- Process PCF0 inputs (Encoder Button) ---
    for (int i = 0; i <= BTN_RIGHT2; i++) {
        if (i == ENC_A || i == ENC_B) continue;
        bool rawHardwareState = (pcf0S & (1 << i)) ? true : false;
        bool currentRawState = rawHardwareState;

        if (currentRawState != lastRawHState0[i]) {
            lastDbncT0[i] = curT;
        }
        lastRawHState0[i] = currentRawState;

        if ((curT - lastDbncT0[i]) > DEBOUNCE_DELAY) {
            bool debouncedState = currentRawState;
            if (debouncedState != prevDbncHState0[i]) {
                if (debouncedState == false) { // Button pressed
                    btnPress0[i] = true; // Set one-shot press flag
                }
                prevDbncHState0[i] = debouncedState;
            }
        }
    }

    // --- Process PCF1 inputs (Navigation buttons) ---
    selectMux(1);
    uint8_t pcf1S = readPCF(PCF1_ADDR);
    for (int i = 0; i < 8; i++) {
        bool rawHardwareState = (pcf1S & (1 << i)) ? true : false;
        bool currentRawState = rawHardwareState;

        if (currentRawState != lastRawHState1[i]) {
            lastDbncT1[i] = curT;
        }
        lastRawHState1[i] = currentRawState;

        if ((curT - lastDbncT1[i]) > DEBOUNCE_DELAY) {
            bool debouncedState = currentRawState;
            if (debouncedState != prevDbncHState1[i]) {
                prevDbncHState1[i] = debouncedState;
                if (debouncedState == false) { // Button pressed
                    btnPress1[i] = true; // Set one-shot flag for this cycle

                    // Auto-repeat logic initiation (only if NOT in overlay or keyboard mode for these buttons)
                    if (!showWifiDisconnectOverlay && currentMenu != WIFI_PASSWORD_INPUT) {
                        bool isListNow = (currentMenu == MAIN_MENU || currentMenu == WIFI_SETUP_MENU);
                        bool isCarouselNow = (currentMenu == GAMES_MENU || currentMenu == TOOLS_MENU || currentMenu == SETTINGS_MENU || currentMenu == UTILITIES_MENU);
                        bool isGridNow = (currentMenu == TOOL_CATEGORY_GRID);
                        bool relevantScrollButton = false;

                        if (isListNow && (i == NAV_UP || i == NAV_DOWN)) relevantScrollButton = true;
                        else if (isCarouselNow && (i == NAV_LEFT || i == NAV_RIGHT)) relevantScrollButton = true;
                        else if (isGridNow && (i == NAV_UP || i == NAV_DOWN || i == NAV_LEFT || i == NAV_RIGHT)) relevantScrollButton = true;
                        
                        if (relevantScrollButton) {
                            isBtnHeld1[i] = true;
                            btnHoldStartT1[i] = curT;
                            lastRepeatT1[i] = curT;
                        }
                    }
                } else { // Button released
                    isBtnHeld1[i] = false; // Stop hold repeat for this button
                }
            }
        }
    }


    // --- Handle Wi-Fi Disconnect Overlay Input (takes precedence) ---
    if (showWifiDisconnectOverlay) {
        bool overlayInputHandledThisFrame = false;
        if (encoder.update(encA_val, encB_val)) {
            if (encoder.position != lastEncoderPosition) {
                disconnectOverlaySelection = 1 - disconnectOverlaySelection;
                lastEncoderPosition = encoder.position;
                overlayInputHandledThisFrame = true;
            }
        }
        if (btnPress1[NAV_LEFT]) {
            disconnectOverlaySelection = 0; // Select X (Cancel)
            btnPress1[NAV_LEFT] = false; // Consume
            overlayInputHandledThisFrame = true;
        }
        if (btnPress1[NAV_RIGHT]) {
            disconnectOverlaySelection = 1; // Select O (Confirm)
            btnPress1[NAV_RIGHT] = false; // Consume
            overlayInputHandledThisFrame = true;
        }

        if (btnPress1[NAV_OK] || btnPress0[ENC_BTN]) {
            if (disconnectOverlaySelection == 1) { // Confirm Disconnect
                Serial.printf("Disconnecting from %s via overlay...\n", currentSsidToConnect);
                WiFi.disconnect(true);
                updateWifiStatusOnDisconnect();
            } else {
                Serial.println("Disconnect overlay: Cancelled.");
            }
            showWifiDisconnectOverlay = false;
            disconnectOverlayAnimatingIn = false;
            disconnectOverlayCurrentScale = 0.0f;
            if (btnPress1[NAV_OK]) btnPress1[NAV_OK] = false; // Consume
            if (btnPress0[ENC_BTN]) btnPress0[ENC_BTN] = false; // Consume
            overlayInputHandledThisFrame = true;

            if (currentMenu == WIFI_SETUP_MENU) {
                initializeCurrentMenu();
                bool foundPrev = false;
                for(int k=0; k < foundWifiNetworksCount; ++k) {
                    if(strcmp(scannedNetworks[k].ssid, currentSsidToConnect) == 0) {
                        wifiMenuIndex = k; foundPrev = true; break;
                    }
                }
                if (!foundPrev) wifiMenuIndex = 0;
                targetWifiListScrollOffset_Y = 0; 
                currentWifiListScrollOffset_Y_anim = 0;
                if (maxMenuItems > 0) wifiListAnim.setTargets(wifiMenuIndex, maxMenuItems);
            }
        }
        if (btnPress1[NAV_BACK]) {
            showWifiDisconnectOverlay = false;
            disconnectOverlayAnimatingIn = false;
            disconnectOverlayCurrentScale = 0.0f;
            btnPress1[NAV_BACK] = false; // Consume
            overlayInputHandledThisFrame = true;
        }
        
        // If overlay handled input, or is simply active, we might want to return
        // or at least prevent further general input processing for this cycle.
        // The btnPress flags are cleared at the end of the main loop.
        if (overlayInputHandledThisFrame) {
             return; // Overlay activity should take full precedence this frame.
        }
    }


    // --- Encoder scroll for general menus (if not overlay) ---
    if (encoder.update(encA_val, encB_val)) {
        int diff = encoder.position - lastEncoderPosition;
        if (diff != 0) {
            if (currentMenu == WIFI_PASSWORD_INPUT) {
                // Keyboard navigation with encoder (unchanged)
                const KeyboardKey* layout = getCurrentKeyboardLayout(currentKeyboardLayer);
                int prevFocusCol = keyboardFocusCol;
                int direction = (diff > 0 ? 1 : -1);
                int nextCol = keyboardFocusCol;
                int attempts = 0;
                do {
                    nextCol = (nextCol + direction + KB_LOGICAL_COLS) % KB_LOGICAL_COLS;
                    const KeyboardKey& key = getKeyFromLayout(layout, keyboardFocusRow, nextCol);
                    if (key.colSpan > 0) {
                        keyboardFocusCol = nextCol;
                        break;
                    }
                    attempts++;
                } while (attempts < KB_LOGICAL_COLS);
                 if (attempts == KB_LOGICAL_COLS) keyboardFocusCol = prevFocusCol;
            } else {
                scrollAct(diff > 0 ? 1 : -1, false, false);
            }
            lastEncoderPosition = encoder.position;
        }
    }

    // --- PCF0 Encoder Button for Keyboard/General OK (if not overlay) ---
    if (btnPress0[ENC_BTN]) {
        if (currentMenu == WIFI_PASSWORD_INPUT) {
            const KeyboardKey* layout = getCurrentKeyboardLayout(currentKeyboardLayer);
            const KeyboardKey& key = getKeyFromLayout(layout, keyboardFocusRow, keyboardFocusCol);
            handleKeyboardInput(key.value);
            btnPress0[ENC_BTN] = false; // Consume for keyboard
        }
        // If not keyboard, ENC_BTN acts as general OK, handled by main loop logic
        // The flag btnPress0[ENC_BTN] will be cleared there if not consumed here.
    }


    // --- PCF1 Navigation Button Processing (if not overlay) ---
    bool isKeyboardActive = (currentMenu == WIFI_PASSWORD_INPUT);

    if (isKeyboardActive) {
        const KeyboardKey* layout = getCurrentKeyboardLayout(currentKeyboardLayer);
        bool focusChangedThisFrame = false; // Track if focus changed by THIS block

        if (btnPress1[NAV_UP]) {
            keyboardFocusRow = (keyboardFocusRow - 1 + KB_ROWS) % KB_ROWS;
            focusChangedThisFrame = true;
            btnPress1[NAV_UP] = false; // Consume the one-shot press
        }
        if (btnPress1[NAV_DOWN]) {
            keyboardFocusRow = (keyboardFocusRow + 1) % KB_ROWS;
            focusChangedThisFrame = true;
            btnPress1[NAV_DOWN] = false; // Consume
        }
        if (btnPress1[NAV_LEFT]) {
            int prevFocusCol = keyboardFocusCol;
            int nextCol = keyboardFocusCol;
            int attempts = 0;
            do {
                nextCol = (nextCol - 1 + KB_LOGICAL_COLS) % KB_LOGICAL_COLS;
                while(nextCol > 0 && getKeyFromLayout(layout, keyboardFocusRow, nextCol).colSpan == 0) {
                    nextCol = (nextCol - 1 + KB_LOGICAL_COLS) % KB_LOGICAL_COLS;
                }
                const KeyboardKey& key = getKeyFromLayout(layout, keyboardFocusRow, nextCol);
                if (key.colSpan > 0) {
                    keyboardFocusCol = nextCol; break;
                } attempts++;
                if (nextCol == prevFocusCol && attempts > 1) break;
            } while (attempts < KB_LOGICAL_COLS * 2);
            focusChangedThisFrame = true;
            btnPress1[NAV_LEFT] = false; // Consume
        }
        if (btnPress1[NAV_RIGHT]) {
            int prevFocusCol = keyboardFocusCol;
            int nextCol = keyboardFocusCol;
            int attempts = 0;
            do {
                const KeyboardKey& currentKeyObj = getKeyFromLayout(layout, keyboardFocusRow, nextCol);
                int span = (currentKeyObj.colSpan > 0) ? currentKeyObj.colSpan : 1;
                nextCol = (nextCol + span) % KB_LOGICAL_COLS;
                const KeyboardKey& key = getKeyFromLayout(layout, keyboardFocusRow, nextCol);
                if (key.colSpan > 0) {
                    keyboardFocusCol = nextCol; break;
                } attempts++;
            } while (attempts < KB_LOGICAL_COLS * 2);
            focusChangedThisFrame = true;
            btnPress1[NAV_RIGHT] = false; // Consume
        }

        if (btnPress1[NAV_OK]) { // NAV_OK for keyboard character press
            const KeyboardKey& key = getKeyFromLayout(layout, keyboardFocusRow, keyboardFocusCol);
            handleKeyboardInput(key.value);
            btnPress1[NAV_OK] = false; // Consume
        }
        if (btnPress1[NAV_A]) { handleKeyboardInput(KB_KEY_SHIFT); btnPress1[NAV_A] = false; /* Consume */ }
        if (btnPress1[NAV_B]) {
            if (currentKeyboardLayer == KB_LAYER_LOWERCASE || currentKeyboardLayer == KB_LAYER_UPPERCASE) handleKeyboardInput(KB_KEY_TO_NUM);
            else if (currentKeyboardLayer == KB_LAYER_NUMBERS) handleKeyboardInput(KB_KEY_TO_SYM);
            else handleKeyboardInput(KB_KEY_TO_LOWER);
            btnPress1[NAV_B] = false; // Consume
        }

        if (focusChangedThisFrame) { // Validate focus after a move
            const KeyboardKey* currentLayout_kb = getCurrentKeyboardLayout(currentKeyboardLayer);
            int attempts = 0;
            int originalFocusCol = keyboardFocusCol; // Store original target col before scan
            while(getKeyFromLayout(currentLayout_kb, keyboardFocusRow, keyboardFocusCol).colSpan == 0 && attempts < KB_LOGICAL_COLS){
                keyboardFocusCol = (keyboardFocusCol + 1) % KB_LOGICAL_COLS; // Scan right for a key start
                attempts++;
            }
            // If scanning right found nothing and wrapped around back to original (or passed it without finding anything)
            // and the original target was also not valid, this logic might need refinement or accept first valid key on row.
            // For now, if it's still bad, it might mean the row is sparse.
            if(getKeyFromLayout(currentLayout_kb, keyboardFocusRow, keyboardFocusCol).colSpan == 0){
                 // Fallback or more complex logic to find *any* valid key on the row
                 // For simplicity, revert if no immediate valid key is found by simple scan.
                 // This area might need more robust handling for very sparse/complex layouts.
            }
        }

    } else { // Not keyboard active: Handle scrolling for directional buttons from one-shot flags
        int scrollDirection = 0;
        bool relevantScrollButton = false;
        bool isListNow = (currentMenu == MAIN_MENU || currentMenu == WIFI_SETUP_MENU);
        bool isCarouselNow = (currentMenu == GAMES_MENU || currentMenu == TOOLS_MENU || currentMenu == SETTINGS_MENU || currentMenu == UTILITIES_MENU);
        bool isGridNow = (currentMenu == TOOL_CATEGORY_GRID);

        if (btnPress1[NAV_UP] && isListNow) { scrollDirection = -1; relevantScrollButton = true; btnPress1[NAV_UP]=false;}
        else if (btnPress1[NAV_DOWN] && isListNow) { scrollDirection = 1; relevantScrollButton = true; btnPress1[NAV_DOWN]=false;}
        else if (btnPress1[NAV_LEFT] && isCarouselNow) { scrollDirection = -1; relevantScrollButton = true; btnPress1[NAV_LEFT]=false;}
        else if (btnPress1[NAV_RIGHT] && isCarouselNow) { scrollDirection = 1; relevantScrollButton = true; btnPress1[NAV_RIGHT]=false;}
        else if (isGridNow) {
            if (btnPress1[NAV_UP]) { scrollDirection = -gridCols; relevantScrollButton = true; btnPress1[NAV_UP]=false;}
            else if (btnPress1[NAV_DOWN]) { scrollDirection = gridCols; relevantScrollButton = true; btnPress1[NAV_DOWN]=false;}
            else if (btnPress1[NAV_LEFT]) { scrollDirection = -1; relevantScrollButton = true; btnPress1[NAV_LEFT]=false;}
            else if (btnPress1[NAV_RIGHT]) { scrollDirection = 1; relevantScrollButton = true; btnPress1[NAV_RIGHT]=false;}
        }
        
        if (relevantScrollButton) {
            scrollAct(scrollDirection, false, false);
        }
    }

    // Handle auto-repeat for scrolling (if button is held, and not keyboard/overlay)
    for(int i=0; i<8; ++i) {
        selectMux(1); 
        uint8_t currentPcf1State = readPCF(PCF1_ADDR);
        bool currentDebouncedPinState = ! (currentPcf1State & (1 << i)); // true if pressed (active low)

        if (currentDebouncedPinState && isBtnHeld1[i] && !isKeyboardActive && !showWifiDisconnectOverlay) {
            if (curT - btnHoldStartT1[i] > REPEAT_INIT_DELAY && curT - lastRepeatT1[i] > REPEAT_INT) {
                int scrollDirectionRepeat = 0;
                bool relevantRepeatButton = false;
                bool isListNow = (currentMenu == MAIN_MENU || currentMenu == WIFI_SETUP_MENU);
                bool isCarouselNow = (currentMenu == GAMES_MENU || currentMenu == TOOLS_MENU || currentMenu == SETTINGS_MENU || currentMenu == UTILITIES_MENU);
                bool isGridNow = (currentMenu == TOOL_CATEGORY_GRID);

                if (isListNow) {
                    if (i == NAV_UP) { scrollDirectionRepeat = -1; relevantRepeatButton = true; }
                    else if (i == NAV_DOWN) { scrollDirectionRepeat = 1; relevantRepeatButton = true; }
                } else if (isCarouselNow) {
                    if (i == NAV_LEFT) { scrollDirectionRepeat = -1; relevantRepeatButton = true; }
                    else if (i == NAV_RIGHT) { scrollDirectionRepeat = 1; relevantRepeatButton = true; }
                } else if (isGridNow) {
                    if (i == NAV_UP) { scrollDirectionRepeat = -gridCols; relevantRepeatButton = true; }
                    else if (i == NAV_DOWN) { scrollDirectionRepeat = gridCols; relevantRepeatButton = true; }
                    else if (i == NAV_LEFT) { scrollDirectionRepeat = -1; relevantRepeatButton = true; }
                    else if (i == NAV_RIGHT) { scrollDirectionRepeat = 1; relevantRepeatButton = true; }
                }

                if (relevantRepeatButton) {
                    scrollAct(scrollDirectionRepeat, false, false);
                    lastRepeatT1[i] = curT;
                }
            }
        }
    }
    // Note: btnPress0[] and btnPress1[] flags are generally cleared at the end of the main loop().
    // If a button's one-shot press was consumed here (e.g., for keyboard focus change), its flag is set to false.
}


// scrollAct function (unchanged)
// ...
void scrollAct(int direction, bool isGridContextFlag_IGNORED, bool isCarouselContextFlag_IGNORED) {
  if (currentMenu == WIFI_PASSWORD_INPUT) return;

  int oldMenuIndexGeneric = menuIndex;
  int oldWifiMenuIndex = wifiMenuIndex;
  bool isCarouselActiveCurrent = (currentMenu == GAMES_MENU || currentMenu == TOOLS_MENU || currentMenu == SETTINGS_MENU || currentMenu == UTILITIES_MENU);
  bool isGridContextCurrent = (currentMenu == TOOL_CATEGORY_GRID);
  bool indexChanged = false;

  if (isGridContextCurrent) {
    if (maxMenuItems <= 0) { menuIndex = 0; targetGridScrollOffset_Y = 0; return; }
    int oldMenuIndexGrid = menuIndex;
    int currentRow = menuIndex / gridCols;
    int currentCol = menuIndex % gridCols;
    int numRows = (maxMenuItems + gridCols - 1) / gridCols;
    int newIndex = menuIndex;
    if (direction == -1) {
      newIndex = menuIndex - 1;
      if (currentCol == 0 && currentRow > 0) newIndex = (currentRow * gridCols) - 1;
      else if (currentCol == 0 && currentRow == 0) newIndex = maxMenuItems - 1;
      if (newIndex < 0) newIndex = maxMenuItems - 1;

    } else if (direction == 1) {
      newIndex = menuIndex + 1;
      if (menuIndex == maxMenuItems - 1) newIndex = 0;
      else if (newIndex >= maxMenuItems) newIndex = 0;

    } else if (direction == -gridCols) {
      if (currentRow > 0) {
        newIndex = menuIndex - gridCols;
      } else {
        int targetColInLastRow = currentCol;
        newIndex = (numRows - 1) * gridCols + targetColInLastRow;
        while (newIndex >= maxMenuItems && newIndex >= 0) {
            newIndex -= gridCols;
            if (newIndex < 0 && maxMenuItems > 0) {
                newIndex = maxMenuItems -1;
                break;
            }
        }
        if (newIndex < 0 && maxMenuItems > 0) newIndex = maxMenuItems -1;
        else if (newIndex < 0 && maxMenuItems == 0) newIndex = 0;
      }
    } else if (direction == gridCols) {
      if (currentRow < numRows - 1 && (menuIndex + gridCols < maxMenuItems)) {
        newIndex = menuIndex + gridCols;
      } else {
        newIndex = currentCol;
        if (newIndex >= maxMenuItems) newIndex = 0;
      }
    }
    menuIndex = newIndex;
    if (maxMenuItems > 0) {
        if (menuIndex >= maxMenuItems) menuIndex = maxMenuItems - 1;
        if (menuIndex < 0) menuIndex = 0;
    } else {
        menuIndex = 0;
    }
    indexChanged = (oldMenuIndexGrid != menuIndex);

    int currentSelectionRow = menuIndex / gridCols;
    const int gridItemRowHeight = GRID_ITEM_H + GRID_ITEM_PADDING_Y;
    int gridVisibleAreaY_start = STATUS_BAR_H + 1 + GRID_ITEM_PADDING_Y;
    int gridVisibleAreaH = 64 - gridVisibleAreaY_start - GRID_ITEM_PADDING_Y;
    int visibleRowsOnGrid = gridVisibleAreaH > 0 ? gridVisibleAreaH / gridItemRowHeight : 1;
    if (visibleRowsOnGrid <= 0) visibleRowsOnGrid = 1;

    int top_row_of_current_targeted_viewport = targetGridScrollOffset_Y / gridItemRowHeight;
    if (currentSelectionRow < top_row_of_current_targeted_viewport) {
      targetGridScrollOffset_Y = currentSelectionRow * gridItemRowHeight;
    } else if (currentSelectionRow >= top_row_of_current_targeted_viewport + visibleRowsOnGrid) {
      targetGridScrollOffset_Y = (currentSelectionRow - visibleRowsOnGrid + 1) * gridItemRowHeight;
    }

    if (targetGridScrollOffset_Y < 0) targetGridScrollOffset_Y = 0;
    int totalGridRows = (maxMenuItems + gridCols - 1) / gridCols;
    int maxPossibleScrollOffsetY = 0;
    if (totalGridRows > visibleRowsOnGrid) {
      maxPossibleScrollOffsetY = (totalGridRows - visibleRowsOnGrid) * gridItemRowHeight;
    }
    if (targetGridScrollOffset_Y > maxPossibleScrollOffsetY) {
      targetGridScrollOffset_Y = maxPossibleScrollOffsetY;
    }
    if (targetGridScrollOffset_Y < 0) targetGridScrollOffset_Y = 0;

  } else if (currentMenu == WIFI_SETUP_MENU) {
    if (maxMenuItems <= 0) { wifiMenuIndex = 0; targetWifiListScrollOffset_Y = 0; return; }
    int oldWifiIdx = wifiMenuIndex;
    wifiMenuIndex += direction;
    if (wifiMenuIndex >= maxMenuItems) wifiMenuIndex = 0;
    else if (wifiMenuIndex < 0) wifiMenuIndex = maxMenuItems - 1;
    indexChanged = (oldWifiIdx != wifiMenuIndex);

    const int item_row_h = WIFI_LIST_ITEM_H;
    int list_start_y_abs = STATUS_BAR_H + 1;
    int list_visible_h = 64 - list_start_y_abs;
    float scroll_to_center_selected_item = (wifiMenuIndex * item_row_h) - (list_visible_h / 2) + (item_row_h / 2);

    if (scroll_to_center_selected_item < 0) scroll_to_center_selected_item = 0;
    float max_scroll_val = (maxMenuItems * item_row_h) - list_visible_h;
    if (max_scroll_val < 0) max_scroll_val = 0;
    if (scroll_to_center_selected_item > max_scroll_val) {
        scroll_to_center_selected_item = max_scroll_val;
    }
    if (scroll_to_center_selected_item < 0) scroll_to_center_selected_item = 0;
    targetWifiListScrollOffset_Y = (int)scroll_to_center_selected_item;

  } else {
    if (maxMenuItems <= 0) { menuIndex = 0; return; }
    int oldMenuIdx = menuIndex;
    menuIndex += direction;
    if (menuIndex >= maxMenuItems) menuIndex = 0;
    else if (menuIndex < 0) menuIndex = maxMenuItems - 1;
    indexChanged = (oldMenuIdx != menuIndex);
  }

  bool actualIndexDidChange = false;
  if (currentMenu == WIFI_SETUP_MENU) {
    actualIndexDidChange = (oldWifiMenuIndex != wifiMenuIndex);
  } else {
    actualIndexDidChange = (oldMenuIndexGeneric != menuIndex);
  }

  if (actualIndexDidChange || isGridContextCurrent || currentMenu == WIFI_SETUP_MENU || isCarouselActiveCurrent) {
    if (currentMenu == MAIN_MENU) {
      mainMenuAnim.setTargets(menuIndex, maxMenuItems);
    } else if (currentMenu == WIFI_SETUP_MENU) {
      wifiListAnim.setTargets(wifiMenuIndex, maxMenuItems);
      marqueeActive = false;
      marqueeOffset = 0;
    } else if (isCarouselActiveCurrent) {
      subMenuAnim.setTargets(menuIndex, maxMenuItems);
      marqueeActive = false; marqueeOffset = 0; marqueeScrollLeft = true;
    } else if (isGridContextCurrent) {
      marqueeActive = false; marqueeOffset = 0; marqueeScrollLeft = true;
    }
  }
}

// getPCF1BtnName (unchanged)
// ...
const char* getPCF1BtnName(int pin) {
  if (pin == NAV_OK) return "OK";
  if (pin == NAV_BACK) return "BACK";
  if (pin == NAV_A) return "A";
  if (pin == NAV_B) return "B";
  if (pin == NAV_LEFT) return "LEFT";
  if (pin == NAV_UP) return "UP";
  if (pin == NAV_DOWN) return "DOWN";
  if (pin == NAV_RIGHT) return "RIGHT";
  return "UNK";
}