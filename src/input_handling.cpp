// input_handling.cpp

#include "config.h"
#include "input_handling.h"
#include "pcf_utils.h"
#include "wifi_manager.h" // For attemptWpaWifiConnection (used in handleKeyboardInput)
#include "menu_logic.h"   // For initializeCurrentMenu (used by handleKeyboardInput indirectly)
#include "keyboard_layout.h" // For KB_LOGICAL_COLS, etc. (used in keyboard navigation)

// Static variables for debouncing and button states (as before)
static bool prevDbncHState0[8] = { 1, 1, 1, 1, 1, 1, 1, 1 }, lastRawHState0[8] = { 1, 1, 1, 1, 1, 1, 1, 1 };
static unsigned long lastDbncT0[8] = { 0 };
static bool prevDbncHState1[8] = { 1, 1, 1, 1, 1, 1, 1, 1 }, lastRawHState1[8] = { 1, 1, 1, 1, 1, 1, 1, 1 };
static unsigned long lastDbncT1[8] = { 0 };
static unsigned long btnHoldStartT1[8] = { 0 }, lastRepeatT1[8] = { 0 };
static bool isBtnHeld1[8] = { 0 };


// QuadratureEncoder methods (as before)
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
  if ((n - lastValidTime) < minInterval && consecutiveValid == 0) {} // Debounce logic, no action if too fast unless consecutive
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
    consecutiveValid = 0; // Reset if sequence broken
  }
  lastState = curSt;
  return false;
}

// setupInputs (as before)
void setupInputs() {
  selectMux(0);
  uint8_t initialPcf0State = readPCF(PCF0_ADDR);
  encoder.init(!(initialPcf0State & (1 << ENC_A)), !(initialPcf0State & (1 << ENC_B)));
}

// Helper function for keyboard input (as provided in the previous step)
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
                        addOrUpdateKnownNetwork(currentSsidToConnect, wifiPasswordInput, true); // Correct: new password, reset fails.
                    } else {
                        attemptDirectWifiConnection(currentSsidToConnect); 
                        // addOrUpdateKnownNetwork is called inside attemptDirectWifiConnection
                    }
                    currentMenu = WIFI_CONNECTING;
                    initializeCurrentMenu(); 
                } // ...
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
    selectMux(0); // For Encoder and PCF0 buttons
    uint8_t pcf0S = readPCF(PCF0_ADDR);
    int encA_val = !(pcf0S & (1 << ENC_A));
    int encB_val = !(pcf0S & (1 << ENC_B));

    static int lastEncoderPosition = 0;
    if (encoder.update(encA_val, encB_val)) {
        int diff = encoder.position - lastEncoderPosition;
        if (diff != 0) {
            if (currentMenu == WIFI_PASSWORD_INPUT) {
                const KeyboardKey* layout = getCurrentKeyboardLayout(currentKeyboardLayer);
                int prevFocusCol = keyboardFocusCol;
                int direction = (diff > 0 ? 1 : -1);
                
                // Try to move column first
                int nextCol = keyboardFocusCol;
                int attempts = 0;
                do {
                    nextCol = (nextCol + direction + KB_LOGICAL_COLS) % KB_LOGICAL_COLS;
                    const KeyboardKey& key = getKeyFromLayout(layout, keyboardFocusRow, nextCol);
                    if (key.colSpan > 0) { // Found a valid key start
                        keyboardFocusCol = nextCol;
                        break;
                    }
                    attempts++;
                } while (attempts < KB_LOGICAL_COLS);
                 if (attempts == KB_LOGICAL_COLS) keyboardFocusCol = prevFocusCol; // No valid move in col

            } else {
                scrollAct(diff > 0 ? 1 : -1, false, false);
            }
            lastEncoderPosition = encoder.position;
        }
    }

    // PCF0 Button Logic (Encoder Button)
    for (int i = 0; i <= BTN_RIGHT2; i++) { // Assuming BTN_RIGHT2 is last used button on PCF0
        if (i == ENC_A || i == ENC_B) continue; // Skip encoder pins
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
                    btnPress0[i] = true;
                    if (i == ENC_BTN && currentMenu == WIFI_PASSWORD_INPUT) {
                        const KeyboardKey* layout = getCurrentKeyboardLayout(currentKeyboardLayer);
                        const KeyboardKey& key = getKeyFromLayout(layout, keyboardFocusRow, keyboardFocusCol);
                        handleKeyboardInput(key.value);
                        btnPress0[i] = false; // Consume ENC_BTN press for keyboard
                    }
                }
                prevDbncHState0[i] = debouncedState;
            }
        }
    }

    // PCF1 Button Logic (NAV_LEFT, NAV_RIGHT, etc.)
    selectMux(1); // MUX channel for PCF1
    uint8_t pcf1S = readPCF(PCF1_ADDR);
    for (int i = 0; i < 8; i++) { // Iterate all 8 pins of PCF1
        bool rawHardwareState = (pcf1S & (1 << i)) ? true : false;
        bool currentRawState = rawHardwareState;
        if (currentRawState != lastRawHState1[i]) {
            lastDbncT1[i] = curT;
        }
        lastRawHState1[i] = currentRawState;

        if ((curT - lastDbncT1[i]) > DEBOUNCE_DELAY) {
            bool debouncedState = currentRawState;
            int scrollDirection = 0;
            bool relevantScrollButton = false;

            bool isListNow = (currentMenu == MAIN_MENU || currentMenu == WIFI_SETUP_MENU);
            bool isCarouselNow = (currentMenu == GAMES_MENU || currentMenu == TOOLS_MENU || currentMenu == SETTINGS_MENU || currentMenu == UTILITIES_MENU);
            bool isGridNow = (currentMenu == TOOL_CATEGORY_GRID);
            bool isKeyboardActive = (currentMenu == WIFI_PASSWORD_INPUT);

            if (debouncedState != prevDbncHState1[i]) { // State changed
                prevDbncHState1[i] = debouncedState;
                if (debouncedState == false) { // Button pressed
                    btnPress1[i] = true; // Set one-shot press flag for OK/BACK (unless keyboard handles it)

                    if (isKeyboardActive) {
                        const KeyboardKey* layout = getCurrentKeyboardLayout(currentKeyboardLayer);
                        int prevFocusRow = keyboardFocusRow;
                        int prevFocusCol = keyboardFocusCol;
                        bool focusChanged = false;
                        int direction = 0; // For left/right column movement

                        if (i == NAV_UP)   { keyboardFocusRow = (keyboardFocusRow - 1 + KB_ROWS) % KB_ROWS; focusChanged = true; }
                        else if (i == NAV_DOWN) { keyboardFocusRow = (keyboardFocusRow + 1) % KB_ROWS; focusChanged = true; }
                        else if (i == NAV_LEFT) { direction = -1; focusChanged = true; }
                        else if (i == NAV_RIGHT) { direction = 1; focusChanged = true; }
                        else if (i == NAV_OK) {
                            const KeyboardKey& key = getKeyFromLayout(layout, keyboardFocusRow, keyboardFocusCol);
                            handleKeyboardInput(key.value);
                            btnPress1[i] = false; // Consume OK for keyboard
                        }
                        else if (i == NAV_A) { handleKeyboardInput(KB_KEY_SHIFT); } // A for Shift
                        else if (i == NAV_B) { // B for Layer Cycle
                            if (currentKeyboardLayer == KB_LAYER_LOWERCASE || currentKeyboardLayer == KB_LAYER_UPPERCASE) handleKeyboardInput(KB_KEY_TO_NUM);
                            else if (currentKeyboardLayer == KB_LAYER_NUMBERS) handleKeyboardInput(KB_KEY_TO_SYM);
                            else handleKeyboardInput(KB_KEY_TO_LOWER);
                        }

                        if (direction != 0) { // NAV_LEFT or NAV_RIGHT
                            int nextCol = keyboardFocusCol;
                            int attempts = 0;
                            do {
                                const KeyboardKey& currentKeyObj = getKeyFromLayout(layout, keyboardFocusRow, nextCol);
                                int span = (currentKeyObj.colSpan > 0) ? currentKeyObj.colSpan : 1;
                                if (direction == 1) nextCol = (nextCol + span);
                                else { // direction == -1
                                    // To move left correctly, we need to find the start of the key to the left
                                    // This is a bit simplified; a robust solution would iterate backward, checking colSpans.
                                    // For now, simple decrement and find start.
                                    nextCol = (nextCol - 1 + KB_LOGICAL_COLS) % KB_LOGICAL_COLS;
                                    // Find the actual start of the key at this new 'nextCol'
                                    while(nextCol > 0 && getKeyFromLayout(layout, keyboardFocusRow, nextCol).colSpan == 0) {
                                        nextCol = (nextCol - 1 + KB_LOGICAL_COLS) % KB_LOGICAL_COLS;
                                    }
                                }
                                nextCol %= KB_LOGICAL_COLS; // Ensure wrap around

                                const KeyboardKey& key = getKeyFromLayout(layout, keyboardFocusRow, nextCol);
                                if (key.colSpan > 0) { // Found a valid key start
                                    keyboardFocusCol = nextCol;
                                    break;
                                }
                                attempts++;
                                if (direction == -1 && nextCol == prevFocusCol && attempts > 1) break; // Avoid infinite loop on left with single valid key
                            } while (attempts < KB_LOGICAL_COLS * 2); // Allow more attempts for complex spans
                            if (attempts >= KB_LOGICAL_COLS *2 && keyboardFocusCol == prevFocusCol) {
                                // Could not find a new key, maybe wrap row or stay
                            }
                        }
                         // After row/col change, ensure focusCol is on a valid key start
                        if(focusChanged){
                            const KeyboardKey* currentLayout = getCurrentKeyboardLayout(currentKeyboardLayer);
                            int attempts = 0;
                            while(getKeyFromLayout(currentLayout, keyboardFocusRow, keyboardFocusCol).colSpan == 0 && attempts < KB_LOGICAL_COLS){
                                keyboardFocusCol = (keyboardFocusCol + 1) % KB_LOGICAL_COLS; // Scan right for a key start
                                attempts++;
                            }
                            if(getKeyFromLayout(currentLayout, keyboardFocusRow, keyboardFocusCol).colSpan == 0){ // Still no valid key start
                                keyboardFocusCol = prevFocusCol; // Revert if stuck
                                keyboardFocusRow = prevFocusRow;
                            }
                        }


                    } else { // Not keyboard active: Handle scrolling for directional buttons
                        if (isListNow) {
                            if (i == NAV_UP) { scrollDirection = -1; relevantScrollButton = true; }
                            else if (i == NAV_DOWN) { scrollDirection = 1; relevantScrollButton = true; }
                        } else if (isCarouselNow) {
                            if (i == NAV_LEFT) { scrollDirection = -1; relevantScrollButton = true; }
                            else if (i == NAV_RIGHT) { scrollDirection = 1; relevantScrollButton = true; }
                        } else if (isGridNow) {
                            if (i == NAV_UP) { scrollDirection = -gridCols; relevantScrollButton = true; }
                            else if (i == NAV_DOWN) { scrollDirection = gridCols; relevantScrollButton = true; }
                            else if (i == NAV_LEFT) { scrollDirection = -1; relevantScrollButton = true; }
                            else if (i == NAV_RIGHT) { scrollDirection = 1; relevantScrollButton = true; }
                        }

                        if (relevantScrollButton) {
                            scrollAct(scrollDirection, false, false); // scrollAct determines detailed context
                            isBtnHeld1[i] = true; // For auto-repeat
                            btnHoldStartT1[i] = curT;
                            lastRepeatT1[i] = curT;
                        }
                    }
                } else {  // Button released
                    if (isBtnHeld1[i] && !isKeyboardActive) { // Only reset hold for non-keyboard contexts
                        isBtnHeld1[i] = false;
                    }
                }
            } else if (debouncedState == false && isBtnHeld1[i] && !isKeyboardActive) { // Button held down (for auto-repeat, non-keyboard)
                if (curT - btnHoldStartT1[i] > REPEAT_INIT_DELAY && curT - lastRepeatT1[i] > REPEAT_INT) {
                    scrollDirection = 0;
                    relevantScrollButton = false;

                    if (isListNow) {
                        if (i == NAV_UP) { scrollDirection = -1; relevantScrollButton = true; }
                        else if (i == NAV_DOWN) { scrollDirection = 1; relevantScrollButton = true; }
                    } else if (isCarouselNow) {
                        if (i == NAV_LEFT) { scrollDirection = -1; relevantScrollButton = true; }
                        else if (i == NAV_RIGHT) { scrollDirection = 1; relevantScrollButton = true; }
                    } else if (isGridNow) {
                        if (i == NAV_UP) { scrollDirection = -gridCols; relevantScrollButton = true; }
                        else if (i == NAV_DOWN) { scrollDirection = gridCols; relevantScrollButton = true; }
                        else if (i == NAV_LEFT) { scrollDirection = -1; relevantScrollButton = true; }
                        else if (i == NAV_RIGHT) { scrollDirection = 1; relevantScrollButton = true; }
                    }

                    if (relevantScrollButton) {
                        scrollAct(scrollDirection, false, false);
                        lastRepeatT1[i] = curT;
                    }
                }
            }
        }
    }
}

// THIS IS THE scrollAct FUNCTION YOU PROVIDED
void scrollAct(int direction, bool isGridContextFlag_IGNORED, bool isCarouselContextFlag_IGNORED) {
  // This function should NOT be called if currentMenu is WIFI_PASSWORD_INPUT
  if (currentMenu == WIFI_PASSWORD_INPUT) return;

  int oldMenuIndexGeneric = menuIndex;
  int oldWifiMenuIndex = wifiMenuIndex;
  bool isCarouselActiveCurrent = (currentMenu == GAMES_MENU || currentMenu == TOOLS_MENU || currentMenu == SETTINGS_MENU || currentMenu == UTILITIES_MENU); // Added UTILITIES_MENU
  bool isGridContextCurrent = (currentMenu == TOOL_CATEGORY_GRID);
  bool indexChanged = false;

  if (isGridContextCurrent) {
    if (maxMenuItems <= 0) { menuIndex = 0; targetGridScrollOffset_Y = 0; return; }
    int oldMenuIndexGrid = menuIndex;
    int currentRow = menuIndex / gridCols;
    int currentCol = menuIndex % gridCols;
    int numRows = (maxMenuItems + gridCols - 1) / gridCols;
    int newIndex = menuIndex;
    if (direction == -1) { // Left within grid
      newIndex = menuIndex - 1;
      // If at start of a row (not first row), wrap to end of previous row
      if (currentCol == 0 && currentRow > 0) newIndex = (currentRow * gridCols) - 1;
      // If at very first item, wrap to last item
      else if (currentCol == 0 && currentRow == 0) newIndex = maxMenuItems - 1;
      // General wrap if newIndex goes below 0 (can happen if menuIndex was 0)
      if (newIndex < 0) newIndex = maxMenuItems - 1;

    } else if (direction == 1) { // Right within grid
      newIndex = menuIndex + 1;
      // If at end of a row (not last row), wrap to start of next row
      if (currentCol == gridCols - 1 && newIndex < maxMenuItems && (newIndex / gridCols) > currentRow) {
         // This case means we are trying to go from (say) item 1 of 2xN grid to item 2,
         // but newIndex is already on the next row. Default wrap works.
      }
      // If at very last item, wrap to first item
      else if (menuIndex == maxMenuItems - 1) newIndex = 0;
      // If moving right pushes past maxMenuItems (e.g., last row not full)
      else if (newIndex >= maxMenuItems) newIndex = 0; // Wrap to first item

    } else if (direction == -gridCols) { // Up within grid (moving by 'number of columns')
      if (currentRow > 0) {
        newIndex = menuIndex - gridCols;
      } else { // At the top row, wrap to the bottom row, same column (or closest if last row not full)
        int targetColInLastRow = currentCol;
        newIndex = (numRows - 1) * gridCols + targetColInLastRow;
        // If calculated index is beyond actual items (last row not full), adjust
        while (newIndex >= maxMenuItems && newIndex >= 0) { // also check newIndex >=0 to prevent infinite loop if maxMenuItems is 0 or error
            newIndex -= gridCols; // Go up one row effectively from the (too far) bottom
            if (newIndex < 0 && maxMenuItems > 0) { // If we went too far up from a sparse last row attempt
                newIndex = maxMenuItems -1; // Settle on the very last item
                break;
            }
        }
        if (newIndex < 0 && maxMenuItems > 0) newIndex = maxMenuItems -1; // Fallback if still negative
        else if (newIndex < 0 && maxMenuItems == 0) newIndex = 0; // Edge case
      }
    } else if (direction == gridCols) {  // Down within grid
      if (currentRow < numRows - 1 && (menuIndex + gridCols < maxMenuItems)) {
        newIndex = menuIndex + gridCols;
      } else { // At the bottom row (or would go past maxItems), wrap to top row, same column
        newIndex = currentCol; // Target column in the first row
        // If the first row doesn't have this column (e.g. 1 item in 2-col grid), adjust.
        if (newIndex >= maxMenuItems) newIndex = 0; // Default to first item
      }
    }
    menuIndex = newIndex;
    // Final bounds check for menuIndex after navigation
    if (maxMenuItems > 0) {
        if (menuIndex >= maxMenuItems) menuIndex = maxMenuItems - 1; // Cap at max
        if (menuIndex < 0) menuIndex = 0;                       // Floor at 0
    } else {
        menuIndex = 0; // No items, index must be 0
    }
    indexChanged = (oldMenuIndexGrid != menuIndex);

    // Scroll offset calculation for grid (copied from your provided function)
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
    if (targetGridScrollOffset_Y < 0) targetGridScrollOffset_Y = 0; // Redundant but safe

  } else if (currentMenu == WIFI_SETUP_MENU) {
    if (maxMenuItems <= 0) { wifiMenuIndex = 0; targetWifiListScrollOffset_Y = 0; return; }
    int oldWifiIdx = wifiMenuIndex;
    wifiMenuIndex += direction;
    if (wifiMenuIndex >= maxMenuItems) wifiMenuIndex = 0; // Wrap to top
    else if (wifiMenuIndex < 0) wifiMenuIndex = maxMenuItems - 1; // Wrap to bottom
    indexChanged = (oldWifiIdx != wifiMenuIndex);

    // Scroll offset calculation for Wi-Fi list (copied from your provided function)
    const int item_row_h = WIFI_LIST_ITEM_H;
    int list_start_y_abs = STATUS_BAR_H + 1;
    int list_visible_h = 64 - list_start_y_abs; // Assuming main display height 64
    // Calculate target scroll to center the selected item
    float scroll_to_center_selected_item = (wifiMenuIndex * item_row_h) - (list_visible_h / 2) + (item_row_h / 2);

    // Clamp scroll_to_center_selected_item
    if (scroll_to_center_selected_item < 0) scroll_to_center_selected_item = 0;
    float max_scroll_val = (maxMenuItems * item_row_h) - list_visible_h;
    if (max_scroll_val < 0) max_scroll_val = 0; // Handles case where list is shorter than visible area
    if (scroll_to_center_selected_item > max_scroll_val) {
        scroll_to_center_selected_item = max_scroll_val;
    }
    // Final safety check, though redundant if logic above is correct
    if (scroll_to_center_selected_item < 0) scroll_to_center_selected_item = 0;
    targetWifiListScrollOffset_Y = (int)scroll_to_center_selected_item;


  } else {  // Main Menu or Carousel (Games, Tools, Settings, Utilities)
    if (maxMenuItems <= 0) { menuIndex = 0; return; } // No items to navigate
    int oldMenuIdx = menuIndex;
    menuIndex += direction;
    if (menuIndex >= maxMenuItems) menuIndex = 0; // Wrap to top
    else if (menuIndex < 0) menuIndex = maxMenuItems - 1; // Wrap to bottom
    indexChanged = (oldMenuIdx != menuIndex);
  }

  // Determine if the active index actually changed
  bool actualIndexDidChange = false;
  if (currentMenu == WIFI_SETUP_MENU) {
    actualIndexDidChange = (oldWifiMenuIndex != wifiMenuIndex);
  } else { // Covers MAIN_MENU, TOOL_CATEGORY_GRID, and CAROUSEL menus
    actualIndexDidChange = (oldMenuIndexGeneric != menuIndex);
  }


  // Update animation targets if an index changed OR
  // if it's a scrollable context that might need marquee reset even if index didn't change at edge.
  if (actualIndexDidChange || isGridContextCurrent || currentMenu == WIFI_SETUP_MENU || isCarouselActiveCurrent) {
    if (currentMenu == MAIN_MENU) {
      mainMenuAnim.setTargets(menuIndex, maxMenuItems);
    } else if (currentMenu == WIFI_SETUP_MENU) {
      wifiListAnim.setTargets(wifiMenuIndex, maxMenuItems); // For scale animation of items
      marqueeActive = false; // Reset marquee for Wi-Fi list on scroll
      marqueeOffset = 0;
    } else if (isCarouselActiveCurrent) {
      subMenuAnim.setTargets(menuIndex, maxMenuItems);
      marqueeActive = false; marqueeOffset = 0; marqueeScrollLeft = true; // Reset marquee for carousel
    } else if (isGridContextCurrent) {
      marqueeActive = false; marqueeOffset = 0; marqueeScrollLeft = true; // Reset marquee for grid
    }
  }
}


// getPCF1BtnName (as before)
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