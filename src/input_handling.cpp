#include "config.h"
#include "input_handling.h"
#include "pcf_utils.h"
#include "wifi_manager.h"
#include "menu_logic.h"
#include "keyboard_layout.h"

// Static variables for debouncing and button states 
static bool prevDbncHState0[8] = { 1, 1, 1, 1, 1, 1, 1, 1 }, lastRawHState0[8] = { 1, 1, 1, 1, 1, 1, 1, 1 };
static unsigned long lastDbncT0[8] = { 0 };
static bool prevDbncHState1[8] = { 1, 1, 1, 1, 1, 1, 1, 1 }, lastRawHState1[8] = { 1, 1, 1, 1, 1, 1, 1, 1 };
static unsigned long lastDbncT1[8] = { 0 };
static unsigned long btnHoldStartT1[8] = { 0 }, lastRepeatT1[8] = { 0 };
static bool isBtnHeld1[8] = { 0 };


// QuadratureEncoder methods 
void QuadratureEncoder::init(int iA, int iB) {
  position = 0;
  lastState = (iB << 1) | iA;
  lastValidTime = millis();
  consecutiveValid = 0;
}

bool QuadratureEncoder::update(int cA, int cB) {
  int currentState = (cB << 1) | cA;  // Combine current A and B states
  // unsigned long currentTime = millis(); // Only needed if using minInterval for full tick rate limiting

  if (currentState == lastState) {
    return false;  // No change in encoder state, do nothing
  }

  // Determine if the transition is valid CW or CCW based on the previous state
  bool validCW = (currentState == cwTable[lastState]);
  bool validCCW = (currentState == ccwTable[lastState]);

  if (validCW || validCCW) {
    // This is a valid micro-step in a known direction.
    lastState = currentState;  // CRITICAL: Update lastState on every valid micro-step
    consecutiveValid++;

    if (consecutiveValid >= requiredConsecutive) {
      // Enough consecutive valid steps have occurred to register a full tick.
      if (validCW) {
        position++;
      } else {  // validCCW
        position--;
      }
      // lastValidTime = currentTime; // Update if minInterval logic for full ticks is used
      consecutiveValid = 0;  // Reset for the next full tick
      return true;           // A full tick was registered
    }
    // Not enough consecutive steps yet for a full tick, but this step was valid.
    return false;  // No full tick registered yet
  } else {
    // This is an invalid transition (e.g., bounce, or skipped state due to very fast turn).
    // Reset the count of valid steps.
    consecutiveValid = 0;
    // Crucially, update lastState to the new currentState to re-synchronize.
    // This allows the system to correctly interpret subsequent states.
    lastState = currentState;
    return false;  // No full tick registered
  }
}

// setupInputs 
void setupInputs() {
  selectMux(0);
  uint8_t initialPcf0State = readPCF(PCF0_ADDR);
  encoder.init(!(initialPcf0State & (1 << ENC_A)), !(initialPcf0State & (1 << ENC_B)));
}

// handleKeyboardInput 
void handleKeyboardInput(int keyPressedValue) {
  if (wifiPasswordInputCursor < PASSWORD_MAX_LEN && keyPressedValue > 0 && keyPressedValue < 127) {  // Printable ASCII
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
            // For open networks, password is "", add/update if needed
            addOrUpdateKnownNetwork(currentSsidToConnect, "", true);
          }
          currentMenu = WIFI_CONNECTING;
          initializeCurrentMenu(); // This will re-init the new menu
        }
        break;
      case KB_KEY_SHIFT:
        if (capsLockActive) { // If caps lock is on, shift temporarily inverts it
          if (currentKeyboardLayer == KB_LAYER_UPPERCASE) currentKeyboardLayer = KB_LAYER_LOWERCASE;
        } else { // If caps lock is off, shift toggles between lower and upper
          if (currentKeyboardLayer == KB_LAYER_LOWERCASE) currentKeyboardLayer = KB_LAYER_UPPERCASE;
          else if (currentKeyboardLayer == KB_LAYER_UPPERCASE) currentKeyboardLayer = KB_LAYER_LOWERCASE;
        }
        // No focus change on shift, it's a modifier
        break;
      case KB_KEY_CAPSLOCK:
        capsLockActive = !capsLockActive;
        if (capsLockActive) currentKeyboardLayer = KB_LAYER_UPPERCASE;
        else currentKeyboardLayer = KB_LAYER_LOWERCASE;
        // No focus change on caps lock
        break;
      case KB_KEY_TO_NUM:
        currentKeyboardLayer = KB_LAYER_NUMBERS;
        capsLockActive = false; // Turn off caps lock when going to numbers/symbols
        keyboardFocusRow = 0;   // Reset focus
        keyboardFocusCol = 0;
        break;
      case KB_KEY_TO_SYM:
        currentKeyboardLayer = KB_LAYER_SYMBOLS;
        capsLockActive = false;
        keyboardFocusRow = 0;
        keyboardFocusCol = 0;
        break;
      case KB_KEY_TO_LOWER:
        currentKeyboardLayer = KB_LAYER_LOWERCASE;
        // capsLockActive = false; // Caps lock remains if it was on and user explicitly goes to lower via KB_KEY_TO_LOWER
        keyboardFocusRow = 0;
        keyboardFocusCol = 0;
        break;
      case KB_KEY_TO_UPPER:
        currentKeyboardLayer = KB_LAYER_UPPERCASE;
        // capsLockActive state depends on previous state, might remain on if user explicitly selects upper
        keyboardFocusRow = 0;
        keyboardFocusCol = 0;
        break;
      case KB_KEY_CYCLE_LAYOUT: // New case for NAV_B functionality
        if (currentKeyboardLayer == KB_LAYER_LOWERCASE || currentKeyboardLayer == KB_LAYER_UPPERCASE) {
            currentKeyboardLayer = KB_LAYER_NUMBERS;
        } else if (currentKeyboardLayer == KB_LAYER_NUMBERS) {
            currentKeyboardLayer = KB_LAYER_SYMBOLS;
        } else if (currentKeyboardLayer == KB_LAYER_SYMBOLS) {
            currentKeyboardLayer = KB_LAYER_LOWERCASE; // Cycle back to lowercase
        }
        capsLockActive = false; // Always turn off caps lock when cycling to/from num/sym
        keyboardFocusRow = 0;   // Reset focus on layout change
        keyboardFocusCol = 0;
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
                if (debouncedState == false) {
                    btnPress0[i] = true;
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
                if (debouncedState == false) {
                    btnPress1[i] = true;
                    if (!showPromptOverlay && currentMenu != WIFI_PASSWORD_INPUT) { 
                        bool isListNow = (currentMenu == MAIN_MENU || currentMenu == WIFI_SETUP_MENU || currentMenu == FIRMWARE_SD_LIST_MENU);
                        bool isCarouselNow = (currentMenu == GAMES_MENU || currentMenu == TOOLS_MENU || currentMenu == SETTINGS_MENU || currentMenu == UTILITIES_MENU);
                        bool isGridNow = (currentMenu == TOOL_CATEGORY_GRID || currentMenu == FIRMWARE_UPDATE_GRID);
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
                } else { 
                    isBtnHeld1[i] = false;
                }
            }
        }
    }

    if (showPromptOverlay) { 
        bool overlayInputHandledThisFrame = false;
        if (encoder.update(encA_val, encB_val)) {
            if (encoder.position != lastEncoderPosition) {
                promptOverlaySelection = 1 - promptOverlaySelection;
                lastEncoderPosition = encoder.position;
                overlayInputHandledThisFrame = true;
            }
        }
        if (btnPress1[NAV_LEFT]) {
            promptOverlaySelection = 0; 
            btnPress1[NAV_LEFT] = false; 
            overlayInputHandledThisFrame = true;
        }
        if (btnPress1[NAV_RIGHT]) {
            promptOverlaySelection = 1; 
            btnPress1[NAV_RIGHT] = false; 
            overlayInputHandledThisFrame = true;
        }

        if (btnPress1[NAV_OK] || btnPress0[ENC_BTN]) {
            if (promptOverlaySelection == 1) {
                Serial.println("Prompt Overlay: Confirmed.");
                if (promptOverlayConfirmAction) {
                    promptOverlayConfirmAction(); 
                }
                if (currentMenu != promptOverlayActionMenuTarget) {
                    currentMenu = promptOverlayActionMenuTarget;
                }
            } else { 
                Serial.println("Prompt Overlay: Cancelled.");
            }
            showPromptOverlay = false;
            promptOverlayAnimatingIn = false; 
            promptOverlayCurrentScale = 0.0f; 
            promptOverlayConfirmAction = nullptr; 
            if (btnPress1[NAV_OK]) btnPress1[NAV_OK] = false; 
            if (btnPress0[ENC_BTN]) btnPress0[ENC_BTN] = false; 
            overlayInputHandledThisFrame = true;
        }
        if (btnPress1[NAV_BACK]) {
            Serial.println("Prompt Overlay: Backed out.");
            showPromptOverlay = false;
            promptOverlayAnimatingIn = false;
            promptOverlayCurrentScale = 0.0f;
            promptOverlayConfirmAction = nullptr;
            btnPress1[NAV_BACK] = false; 
            overlayInputHandledThisFrame = true;
        }
        if (overlayInputHandledThisFrame) {
             return; 
        }
    }

    if (encoder.update(encA_val, encB_val)) {
        int diff = encoder.position - lastEncoderPosition;
        if (diff != 0) {
            if (currentMenu == WIFI_PASSWORD_INPUT) {
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

    if (btnPress0[ENC_BTN]) { 
        if (currentMenu == WIFI_PASSWORD_INPUT) {
            const KeyboardKey* layout = getCurrentKeyboardLayout(currentKeyboardLayer);
            const KeyboardKey& key = getKeyFromLayout(layout, keyboardFocusRow, keyboardFocusCol);
            handleKeyboardInput(key.value);
            btnPress0[ENC_BTN] = false; 
        }
    }

    bool isKeyboardActive = (currentMenu == WIFI_PASSWORD_INPUT);
    // bool isOverlayActive = showPromptOverlay; // Already declared above and handled

    if (isKeyboardActive) {
        const KeyboardKey* layout = getCurrentKeyboardLayout(currentKeyboardLayer);
        bool focusChangedThisFrame = false; 

        // --- BACK BUTTON HANDLING FOR KEYBOARD SCREEN ---
        if (btnPress1[NAV_BACK]) {
            handleMenuBackNavigation(); // This function should handle going from WIFI_PASSWORD_INPUT to WIFI_SETUP_MENU
            btnPress1[NAV_BACK] = false; 
            return; // Input handled, exit updateInputs for this cycle
        }
        // --- END OF BACK BUTTON HANDLING ---


        if (btnPress1[NAV_UP]) {
            keyboardFocusRow = (keyboardFocusRow - 1 + KB_ROWS) % KB_ROWS;
            focusChangedThisFrame = true;
            btnPress1[NAV_UP] = false; 
        }
        if (btnPress1[NAV_DOWN]) {
            keyboardFocusRow = (keyboardFocusRow + 1) % KB_ROWS;
            focusChangedThisFrame = true;
            btnPress1[NAV_DOWN] = false; 
        }
        if (btnPress1[NAV_LEFT]) { 
            const KeyboardKey* currentLayout = getCurrentKeyboardLayout(currentKeyboardLayer); 
            int prevFocusCol = keyboardFocusCol;
            int nextCol = keyboardFocusCol;
            int attempts = 0;
            do {
                nextCol = (nextCol - 1 + KB_LOGICAL_COLS) % KB_LOGICAL_COLS; 
                const KeyboardKey& key = getKeyFromLayout(currentLayout, keyboardFocusRow, nextCol);
                if (key.colSpan > 0) { 
                    keyboardFocusCol = nextCol;
                    break;
                }
                attempts++;
            } while (attempts < KB_LOGICAL_COLS);
            if (attempts == KB_LOGICAL_COLS) keyboardFocusCol = prevFocusCol; 
            focusChangedThisFrame = true; btnPress1[NAV_LEFT] = false; 
        }
        if (btnPress1[NAV_RIGHT]) { 
            const KeyboardKey* currentLayout = getCurrentKeyboardLayout(currentKeyboardLayer); 
            int prevFocusCol = keyboardFocusCol;
            int nextCol = keyboardFocusCol;
            int attempts = 0;
            do {
                nextCol = (nextCol + 1) % KB_LOGICAL_COLS; 
                const KeyboardKey& key = getKeyFromLayout(currentLayout, keyboardFocusRow, nextCol);
                if (key.colSpan > 0) { 
                    keyboardFocusCol = nextCol;
                    break;
                }
                attempts++;
            } while (attempts < KB_LOGICAL_COLS);
            if (attempts == KB_LOGICAL_COLS) keyboardFocusCol = prevFocusCol; 
            focusChangedThisFrame = true; btnPress1[NAV_RIGHT] = false;
        }

        if (btnPress1[NAV_OK]) { 
            const KeyboardKey& key = getKeyFromLayout(layout, keyboardFocusRow, keyboardFocusCol);
            handleKeyboardInput(key.value);
            btnPress1[NAV_OK] = false; 
        }
        if (btnPress1[NAV_A]) { 
            handleKeyboardInput(KB_KEY_SHIFT); 
            btnPress1[NAV_A] = false; 
        }
        if (btnPress1[NAV_B]) { 
            handleKeyboardInput(KB_KEY_CYCLE_LAYOUT); 
            btnPress1[NAV_B] = false; 
        }

        if (focusChangedThisFrame) { 
            const KeyboardKey& key = getKeyFromLayout(getCurrentKeyboardLayout(currentKeyboardLayer), keyboardFocusRow, keyboardFocusCol);
            if (key.colSpan == 0) { 
                for (int c = keyboardFocusCol; c >= 0; --c) {
                    const KeyboardKey& scanKey = getKeyFromLayout(getCurrentKeyboardLayout(currentKeyboardLayer), keyboardFocusRow, c);
                    if (scanKey.colSpan > 0 && (c + scanKey.colSpan > keyboardFocusCol)) {
                        keyboardFocusCol = c;
                        break;
                    }
                }
            }
        }
    } else if (!showPromptOverlay) { // Not keyboard active and not overlay active
        int scrollDirection = 0;
        bool relevantScrollButton = false;
        bool isListNow = (currentMenu == MAIN_MENU || currentMenu == WIFI_SETUP_MENU || currentMenu == FIRMWARE_SD_LIST_MENU); 
        bool isCarouselNow = (currentMenu == GAMES_MENU || currentMenu == TOOLS_MENU || currentMenu == SETTINGS_MENU || currentMenu == UTILITIES_MENU);
        bool isGridNow = (currentMenu == TOOL_CATEGORY_GRID || currentMenu == FIRMWARE_UPDATE_GRID); 

        if (btnPress1[NAV_UP]) {
            if (isListNow) { scrollDirection = -1; relevantScrollButton = true;}
            else if (isGridNow) { scrollDirection = -gridCols; relevantScrollButton = true; }
            if(relevantScrollButton) btnPress1[NAV_UP]=false;
        }
        else if (btnPress1[NAV_DOWN]) {
            if (isListNow) { scrollDirection = 1; relevantScrollButton = true; }
            else if (isGridNow) { scrollDirection = gridCols; relevantScrollButton = true; }
            if(relevantScrollButton) btnPress1[NAV_DOWN]=false;
        }
        else if (btnPress1[NAV_LEFT]) {
            if (isCarouselNow || isGridNow) { scrollDirection = -1; relevantScrollButton = true; }
            if(relevantScrollButton) btnPress1[NAV_LEFT]=false;
        }
        else if (btnPress1[NAV_RIGHT]) {
            if (isCarouselNow || isGridNow) { scrollDirection = 1; relevantScrollButton = true; }
            if(relevantScrollButton) btnPress1[NAV_RIGHT]=false;
        }
        
        if (relevantScrollButton) {
            scrollAct(scrollDirection, false, false);
        }
    }

    // Handle auto-repeat for scrolling
    for(int i=0; i<8; ++i) {
        selectMux(1); 
        uint8_t currentPcf1State = readPCF(PCF1_ADDR);
        bool currentDebouncedPinState = ! (currentPcf1State & (1 << i)); 

        if (currentDebouncedPinState && isBtnHeld1[i] && !isKeyboardActive && !showPromptOverlay) {
            if (curT - btnHoldStartT1[i] > REPEAT_INIT_DELAY && curT - lastRepeatT1[i] > REPEAT_INT) {
                int scrollDirectionRepeat = 0;
                bool relevantRepeatButton = false;
                bool isListNow = (currentMenu == MAIN_MENU || currentMenu == WIFI_SETUP_MENU || currentMenu == FIRMWARE_SD_LIST_MENU); 
                bool isCarouselNow = (currentMenu == GAMES_MENU || currentMenu == TOOLS_MENU || currentMenu == SETTINGS_MENU || currentMenu == UTILITIES_MENU);
                bool isGridNow = (currentMenu == TOOL_CATEGORY_GRID || currentMenu == FIRMWARE_UPDATE_GRID); 

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
}


// scrollAct function
void scrollAct(int direction, bool isGridContextFlag_IGNORED, bool isCarouselContextFlag_IGNORED) {
  if (currentMenu == WIFI_PASSWORD_INPUT) return;

  int oldMenuIndexGeneric = menuIndex;
  int oldWifiMenuIndex = wifiMenuIndex;
  bool isCarouselActiveCurrent = (currentMenu == GAMES_MENU || currentMenu == TOOLS_MENU || currentMenu == SETTINGS_MENU || currentMenu == UTILITIES_MENU);
  bool isGridContextCurrent = (currentMenu == TOOL_CATEGORY_GRID);
  bool indexChanged = false;

  if (isGridContextCurrent) {
    if (maxMenuItems <= 0) {
      menuIndex = 0;
      targetGridScrollOffset_Y = 0;
      return;
    }
    int oldMenuIndexGrid = menuIndex;
    int currentRow = menuIndex / gridCols;
    int currentCol = menuIndex % gridCols;
    int numRowsTotal = (maxMenuItems + gridCols - 1) / gridCols;  // Total number of rows in the grid
    int newIndex = menuIndex;

    if (direction == -1) {  // NAV_LEFT or Encoder CCW in grid
      if (currentCol > 0) {
        newIndex = menuIndex - 1;
      } else {                                  // At the leftmost column
        newIndex = menuIndex + (gridCols - 1);  // Wrap to the rightmost item in the same row
        if (newIndex >= maxMenuItems) {         // If rightmost in row is out of bounds (short row)
          newIndex = maxMenuItems - 1;          // Go to the absolute last item
        }
      }
    } else if (direction == 1) {  // NAV_RIGHT or Encoder CW in grid
      if (currentCol < gridCols - 1 && (menuIndex + 1) < maxMenuItems) {
        newIndex = menuIndex + 1;
      } else {                              // At the rightmost column (or item before it is the last)
        newIndex = menuIndex - currentCol;  // Wrap to the leftmost item in the same row
      }
    } else if (direction == -gridCols) {  // NAV_UP
      if (currentRow > 0) {
        newIndex = menuIndex - gridCols;  // Move up in the same column
      } else {                            // At the top row
        // "Cross-column" wrap: move to the bottom of the opposite column (if 2 columns)
        // or to the bottom of the adjacent column (if more than 2)
        int targetCol = (currentCol + 1) % gridCols;  // Example: from col 0 to col 1, from col 1 to col 0 (for 2 cols)
                                                      // For more cols, this would be (currentCol + gridCols/2) % gridCols or similar,
                                                      // but for 2 cols, (currentCol + 1) % 2 works.

        // For a generic "opposite" or "next logical" column in a multi-column setup:
        // Let's assume for simplicity with 2 columns:
        if (gridCols == 2) {
          targetCol = 1 - currentCol;  // Toggle between 0 and 1
        } else {
          // For >2 cols, this behavior might need more complex definition.
          // For now, let's make it wrap to the last item of the "next" column.
          targetCol = (currentCol + 1) % gridCols;
        }

        // Find the last valid item in that targetCol
        newIndex = (numRowsTotal - 1) * gridCols + targetCol;  // Start at theoretical last row of targetCol
        while (newIndex >= maxMenuItems && newIndex >= 0) {    // If targetCol is short or index out of bounds
          newIndex -= gridCols;                                // Go up one row in targetCol
        }
        if (newIndex < 0 && maxMenuItems > 0) {                          // If calculation went wrong
          newIndex = (numRowsTotal - 1) * gridCols + currentCol;         // Fallback to bottom of current column
          while (newIndex >= maxMenuItems) newIndex = maxMenuItems - 1;  // ensure in bounds
          if (newIndex < 0) newIndex = maxMenuItems - 1;
        } else if (newIndex < 0) {
          newIndex = 0;  // Absolute fallback
        }
      }
    } else if (direction == gridCols) {  // NAV_DOWN
      int potentialNextIndexInSameCol = menuIndex + gridCols;
      if (potentialNextIndexInSameCol < maxMenuItems) {
        newIndex = potentialNextIndexInSameCol;  // Move down in the same column
      } else {                                   // At the bottom of the current column (or it's a short column)
        // "Cross-column" wrap: move to the top of the opposite/next column
        int targetCol;
        if (gridCols == 2) {
          targetCol = 1 - currentCol;  // Toggle between 0 and 1
        } else {
          targetCol = (currentCol + 1) % gridCols;  // Go to next column (wraps around)
        }
        newIndex = targetCol;            // Top of the target column (row 0, col targetCol)
        if (newIndex >= maxMenuItems) {  // If targetCol doesn't have an item at row 0 (e.g. only 1 item in grid)
          newIndex = 0;                  // Fallback: wrap to the very first item
        }
      }
    }

    menuIndex = newIndex;
    if (maxMenuItems > 0) {  // Ensure menuIndex is always valid
      menuIndex = constrain(menuIndex, 0, maxMenuItems - 1);
    } else {
      menuIndex = 0;
    }
    indexChanged = (oldMenuIndexGrid != menuIndex);

    // --- Scroll Y offset calculation (same as before) ---
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
    if (targetGridScrollOffset_Y < 0) targetGridScrollOffset_Y = 0;  // Redundant but safe

  } else if (currentMenu == WIFI_SETUP_MENU || currentMenu == FIRMWARE_SD_LIST_MENU) {  // MODIFIED
    if (maxMenuItems <= 0) {
      wifiMenuIndex = 0;
      targetWifiListScrollOffset_Y = 0;
      return;
    }
    int oldWifiIdx = wifiMenuIndex;
    wifiMenuIndex += direction;
    if (wifiMenuIndex >= maxMenuItems) wifiMenuIndex = 0;
    else if (wifiMenuIndex < 0) wifiMenuIndex = maxMenuItems - 1;
    indexChanged = (oldWifiIdx != wifiMenuIndex);

    const int item_row_h = (currentMenu == WIFI_SETUP_MENU) ? WIFI_LIST_ITEM_H : FW_LIST_ITEM_H;  // Use correct height
    int list_start_y_abs = STATUS_BAR_H + 1;
    int list_visible_h = 64 - list_start_y_abs;
    float scroll_to_center_selected_item = (wifiMenuIndex * item_row_h) - (list_visible_h / 2) + (item_row_h / 2);

    if (scroll_to_center_selected_item < 0) scroll_to_center_selected_item = 0;
    float max_scroll_val = (maxMenuItems * item_row_h) - list_visible_h;
    if (max_scroll_val < 0) max_scroll_val = 0;
    if (scroll_to_center_selected_item > max_scroll_val) {
      scroll_to_center_selected_item = max_scroll_val;
    }
    if (scroll_to_center_selected_item < 0) scroll_to_center_selected_item = 0;  // Redundant, but safe
    targetWifiListScrollOffset_Y = (int)scroll_to_center_selected_item;

  } else {
    if (maxMenuItems <= 0) {
      menuIndex = 0;
      return;
    }
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
    } else if (currentMenu == WIFI_SETUP_MENU || currentMenu == FIRMWARE_SD_LIST_MENU) {  // MODIFIED
      wifiListAnim.setTargets(wifiMenuIndex, maxMenuItems);                               // This uses wifiMenuIndex
      marqueeActive = false;
      marqueeOffset = 0;
    } else if (isCarouselActiveCurrent) {
      subMenuAnim.setTargets(menuIndex, maxMenuItems);
      marqueeActive = false;
      marqueeOffset = 0;
      marqueeScrollLeft = true;
    } else if (isGridContextCurrent) {
      marqueeActive = false;
      marqueeOffset = 0;
      marqueeScrollLeft = true;
    }
  }
}

// getPCF1BtnName 
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