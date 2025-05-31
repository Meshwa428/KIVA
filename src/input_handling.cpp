// input_handling.cpp

#include "config.h"
#include "input_handling.h"
#include "pcf_utils.h"
// #include "menu_logic.h" // Not directly needed for scrollAct's animation calls if anim objects are extern

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

// setupInputs (as before)
void setupInputs() {
  selectMux(0);
  uint8_t initialPcf0State = readPCF(PCF0_ADDR);
  encoder.init(!(initialPcf0State & (1 << ENC_A)), !(initialPcf0State & (1 << ENC_B)));
}

void updateInputs() {
    unsigned long curT = millis();
    selectMux(0); // For Encoder and PCF0 buttons
    uint8_t pcf0S = readPCF(PCF0_ADDR);
    int encA_val = !(pcf0S & (1 << ENC_A)); 
    int encB_val = !(pcf0S & (1 << ENC_B)); 

    // Encoder Logic
    static int lastEncoderPosition = 0; // Keep static for encoder diff
    if (encoder.update(encA_val, encB_val)) {
        int diff = encoder.position - lastEncoderPosition;
        if (diff != 0) {
            // scrollAct will determine context internally based on currentMenu
            scrollAct(diff > 0 ? 1 : -1, false, false); 
            lastEncoderPosition = encoder.position;
        }
    }

    // PCF0 Button Logic (e.g., Encoder Button)
    for (int i = 0; i <= BTN_RIGHT2; i++) { // Assuming BTN_RIGHT2 is the last used button on PCF0
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
                if (debouncedState == false) { // Button pressed (transition from high to low assumed for pull-ups)
                    btnPress0[i] = true;
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

            // Determine context for button actions
            bool isListNow = (currentMenu == MAIN_MENU || currentMenu == WIFI_SETUP_MENU);
            bool isCarouselNow = (currentMenu == GAMES_MENU || currentMenu == TOOLS_MENU || currentMenu == SETTINGS_MENU || currentMenu == UTILITIES_MENU); // Added UTILITIES_MENU
            bool isGridNow = (currentMenu == TOOL_CATEGORY_GRID); 

            if (debouncedState != prevDbncHState1[i]) { // State changed
                prevDbncHState1[i] = debouncedState;
                if (debouncedState == false) { // Button pressed
                    btnPress1[i] = true; // Set one-shot press flag for OK/BACK

                    // Handle scrolling for directional buttons
                    if (isListNow) { 
                        if (i == NAV_UP) { scrollDirection = -1; relevantScrollButton = true; }
                        else if (i == NAV_DOWN) { scrollDirection = 1; relevantScrollButton = true; }
                    } else if (isCarouselNow) { // <--- THIS IS KEY FOR LEFT/RIGHT IN CAROUSEL
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
                } else {  // Button released
                    if (isBtnHeld1[i]) {
                        isBtnHeld1[i] = false; 
                    }
                }
            } else if (debouncedState == false && isBtnHeld1[i]) { // Button held down (for auto-repeat)
                if (curT - btnHoldStartT1[i] > REPEAT_INIT_DELAY && curT - lastRepeatT1[i] > REPEAT_INT) {
                    // Reset scrollDirection and relevantScrollButton for this repeat event
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

void scrollAct(int direction, bool isGridContextFlag_IGNORED, bool isCarouselContextFlag_IGNORED) {
  int oldMenuIndexGeneric = menuIndex;
  int oldWifiMenuIndex = wifiMenuIndex;
  bool isCarouselActiveCurrent = (currentMenu == GAMES_MENU || currentMenu == TOOLS_MENU || currentMenu == SETTINGS_MENU || currentMenu == UTILITIES_MENU); // Added UTILITIES_MENU
  bool isGridContextCurrent = (currentMenu == TOOL_CATEGORY_GRID);
  bool indexChanged = false;

  if (isGridContextCurrent) {
    // ... Grid logic from previous 'perfect' version ...
    // Ensure this block correctly updates menuIndex and targetGridScrollOffset_Y
    // and sets indexChanged = (oldMenuIndexGrid != menuIndex);
    if (maxMenuItems <= 0) { menuIndex = 0; targetGridScrollOffset_Y = 0; return; }
    int oldMenuIndexGrid = menuIndex; 
    int currentRow = menuIndex / gridCols;
    int currentCol = menuIndex % gridCols;
    int numRows = (maxMenuItems + gridCols - 1) / gridCols;
    int newIndex = menuIndex;
    if (direction == -1) { 
      newIndex = menuIndex - 1;
      if (currentCol == 0) { 
        if (currentRow == 0) newIndex = maxMenuItems - 1; 
        else newIndex = (currentRow * gridCols) - 1; 
      }
      if (newIndex < 0) newIndex = maxMenuItems - 1;
    } else if (direction == 1) { 
      newIndex = menuIndex + 1;
      if (menuIndex == maxMenuItems - 1) { 
        newIndex = 0; 
      } else if (currentCol == gridCols - 1) { 
        newIndex = (currentRow + 1) * gridCols; 
        if (newIndex >= maxMenuItems) newIndex = 0; 
      } else if (newIndex >= maxMenuItems) { 
         newIndex = 0; 
      }
    } else if (direction == -gridCols) {  
      if (currentRow > 0) { 
        newIndex = menuIndex - gridCols;
      } else { 
        int targetCol = currentCol - 1;
        if (targetCol < 0) {
          targetCol = gridCols - 1; 
        }
        newIndex = -1; 
        for (int r = numRows - 1; r >= 0; --r) {
          int tempIdx = r * gridCols + targetCol;
          if (tempIdx < maxMenuItems) {
            newIndex = tempIdx;
            break;
          }
        }
        if (newIndex == -1) { 
          newIndex = maxMenuItems -1; 
        }
      }
    } else if (direction == gridCols) {  
      if (currentRow < numRows - 1 && (menuIndex + gridCols < maxMenuItems)) {
        newIndex = menuIndex + gridCols;
      } else { 
        int targetCol = currentCol + 1;
        if (targetCol >= gridCols) {
          targetCol = 0; 
        }
        newIndex = targetCol; 
        if (newIndex >= maxMenuItems) { 
          newIndex = 0; 
        }
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
    // ... WiFi logic from previous 'perfect' version ...
    // Ensure this block correctly updates wifiMenuIndex and targetWifiListScrollOffset_Y
    // and sets indexChanged = (oldWifiMenuIndex != wifiMenuIndex);
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
    targetWifiListScrollOffset_Y = scroll_to_center_selected_item; 


  } else {  // Main Menu or Carousel (Games, Tools, Settings, Utilities)
    if (maxMenuItems <= 0) { menuIndex = 0; return; } // No items to navigate
    int oldMenuIdx = menuIndex;
    menuIndex += direction; 
    if (menuIndex >= maxMenuItems) menuIndex = 0;
    else if (menuIndex < 0) menuIndex = maxMenuItems - 1;
    indexChanged = (oldMenuIdx != menuIndex);
  }

  // Update animation targets or related states
  // This needs to be sensitive to whether the index actually changed for the *active* menu system
  bool actualIndexDidChange = false;
  if (currentMenu == WIFI_SETUP_MENU) {
    actualIndexDidChange = (oldWifiMenuIndex != wifiMenuIndex);
  } else {
    actualIndexDidChange = (oldMenuIndexGeneric != menuIndex);
  }


  // Update animation targets if an index changed OR 
  // if it's a scrollable context that might need marquee reset even if index didn't change at edge.
  if (actualIndexDidChange || isGridContextCurrent || currentMenu == WIFI_SETUP_MENU || isCarouselActiveCurrent) {
    if (currentMenu == MAIN_MENU) {
      mainMenuAnim.setTargets(menuIndex, maxMenuItems);
    } else if (currentMenu == WIFI_SETUP_MENU) {
      wifiListAnim.setTargets(wifiMenuIndex, maxMenuItems); // For scale
      marqueeActive = false; 
      marqueeOffset = 0;
    } else if (isCarouselActiveCurrent) { 
      // THIS IS CRUCIAL FOR CAROUSEL ANIMATION
      subMenuAnim.setTargets(menuIndex, maxMenuItems);
      marqueeActive = false; marqueeOffset = 0; marqueeScrollLeft = true;
    } else if (isGridContextCurrent) { 
      marqueeActive = false; marqueeOffset = 0; marqueeScrollLeft = true;
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