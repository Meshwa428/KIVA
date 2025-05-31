#include "input_handling.h"
#include "pcf_utils.h"      // For selectMux, readPCF
#include "config.h"         // For all defines and externs
#include "menu_logic.h"     // For mainMenuAnim, subMenuAnim if scrollAct modifies them directly

// Debouncing & Auto-repeat variables (static to this file)
static bool prevDbncHState0[8] = {1,1,1,1,1,1,1,1}, lastRawHState0[8] = {1,1,1,1,1,1,1,1};
static unsigned long lastDbncT0[8] = {0};

static bool prevDbncHState1[8] = {1,1,1,1,1,1,1,1}, lastRawHState1[8] = {1,1,1,1,1,1,1,1};
static unsigned long lastDbncT1[8] = {0};

static unsigned long btnHoldStartT1[8] = {0}, lastRepeatT1[8] = {0};
static bool isBtnHeld1[8] = {0};


// QuadratureEncoder methods
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
    // Debounce logic (minInterval approach)
    if ((n - lastValidTime) < minInterval && consecutiveValid == 0) {
        // Too fast, and not part of a sequence, possibly noise.
        // lastState = curSt; // Update lastState to avoid repeated false triggers from noise
        // return false; // Or let it pass to check for valid transitions
    }

    bool vCW = (curSt == cwTable[lastState]);
    bool vCCW = (curSt == ccwTable[lastState]);

    if (vCW || vCCW) {
        consecutiveValid++;
        if (consecutiveValid >= requiredConsecutive) {
            if (vCW) position++;
            else position--;
            lastValidTime = n;
            consecutiveValid = 0; // Reset for next valid move
            lastState = curSt;
            return true;
        }
        // Not enough consecutive yet, but keep lastState as is for next check
    } else {
        // Invalid transition or noise, reset consecutive count
        consecutiveValid = 0;
    }
    lastState = curSt; // Always update last state
    return false;
}


void setupInputs() {
    // Initialize encoder state based on initial PCF0 readings
    selectMux(0); // MUX channel for PCF0 (encoder, buttons)
    uint8_t initialPcf0State = readPCF(PCF0_ADDR);
    encoder.init(!(initialPcf0State & (1 << ENC_A)), !(initialPcf0State & (1 << ENC_B)));
}

void updateInputs() {
    unsigned long curT = millis();
    bool isCarouselActive = (currentMenu == GAMES_MENU || currentMenu == TOOLS_MENU || currentMenu == SETTINGS_MENU);
    bool isGridActive = (currentMenu == TOOL_CATEGORY_GRID);

    // --- PCF0 Inputs (Encoder + Buttons) ---
    selectMux(0);
    uint8_t pcf0S = readPCF(PCF0_ADDR);

    // Encoder
    int encA_val = !(pcf0S & (1 << ENC_A)); // Inverted logic if active low
    int encB_val = !(pcf0S & (1 << ENC_B)); // Inverted logic if active low
    if (encoder.update(encA_val, encB_val)) {
        static int lastEncoderPosition = 0; // encoder.position is already global via extern
        int diff = encoder.position - lastEncoderPosition;
        if (diff != 0) {
            scrollAct(diff > 0 ? 1 : -1, isGridActive, isCarouselActive); // Normalize for grid L/R
            lastEncoderPosition = encoder.position;
        }
    }

    // PCF0 Buttons (ENC_BTN, BTN_AI, BTN_RIGHT1, BTN_RIGHT2)
    for (int i = 0; i <= BTN_RIGHT2; i++) { // Iterate through relevant button pins on PCF0
        if (i == ENC_A || i == ENC_B) continue; // Skip encoder pins

        bool rawHardwareState = (pcf0S & (1 << i)) ? true : false; // true if bit is 1 (high)
        // Assuming buttons pull low when pressed, so pressed state is false
        bool currentRawState = rawHardwareState; // If using pull-ups, pressed is LOW (false)

        if (currentRawState != lastRawHState0[i]) {
            lastDbncT0[i] = curT; // Reset debounce timer
        }
        lastRawHState0[i] = currentRawState;

        if ((curT - lastDbncT0[i]) > DEBOUNCE_DELAY) {
            // Debounced state is same as currentRawState after delay
            bool debouncedState = currentRawState;
            if (debouncedState != prevDbncHState0[i]) { // State changed
                if (debouncedState == false) { // Button pressed (went from high to low)
                    btnPress0[i] = true;
                    // Serial.printf("PCF0 Btn %d Pressed\n", i);
                }
                // else button released, no action for btnPress0 flag
                prevDbncHState0[i] = debouncedState;
            }
        }
    }

    // --- PCF1 Inputs (Navigation Buttons) ---
    selectMux(1);
    uint8_t pcf1S = readPCF(PCF1_ADDR);

    for (int i = 0; i < 8; i++) { // Iterate through all 8 pins of PCF1
        bool rawHardwareState = (pcf1S & (1 << i)) ? true : false;
        bool currentRawState = rawHardwareState; // Assuming pull-ups, pressed is LOW (false)

        if (currentRawState != lastRawHState1[i]) {
            lastDbncT1[i] = curT;
        }
        lastRawHState1[i] = currentRawState;

        if ((curT - lastDbncT1[i]) > DEBOUNCE_DELAY) {
            bool debouncedState = currentRawState;
            int scrollDirection = 0;
            bool relevantScrollButton = false;

            if (debouncedState != prevDbncHState1[i]) { // State changed
                prevDbncHState1[i] = debouncedState;
                if (debouncedState == false) { // Button Pressed
                    btnPress1[i] = true;
                    // Serial.printf("PCF1 Btn %d (%s) Pressed\n", i, getPCF1BtnName(i));

                    // Determine scroll direction for relevant buttons
                    if (currentMenu == MAIN_MENU) {
                        if (i == NAV_UP) { scrollDirection = -1; relevantScrollButton = true; }
                        else if (i == NAV_DOWN) { scrollDirection = 1; relevantScrollButton = true; }
                    } else if (isCarouselActive) {
                        if (i == NAV_LEFT) { scrollDirection = -1; relevantScrollButton = true; }
                        else if (i == NAV_RIGHT) { scrollDirection = 1; relevantScrollButton = true; }
                    } else if (isGridActive) {
                        if (i == NAV_UP) { scrollDirection = -gridCols; relevantScrollButton = true; }
                        else if (i == NAV_DOWN) { scrollDirection = gridCols; relevantScrollButton = true; }
                        else if (i == NAV_LEFT) { scrollDirection = -1; relevantScrollButton = true; }
                        else if (i == NAV_RIGHT) { scrollDirection = 1; relevantScrollButton = true; }
                    }

                    if (relevantScrollButton) {
                        scrollAct(scrollDirection, isGridActive, isCarouselActive);
                        isBtnHeld1[i] = true; // Start hold tracking
                        btnHoldStartT1[i] = curT;
                        lastRepeatT1[i] = curT;
                    }
                } else { // Button Released
                    if (isBtnHeld1[i]) {
                        isBtnHeld1[i] = false; // Stop hold tracking
                        // Serial.printf("PCF1 Btn %d (%s) Released\n", i, getPCF1BtnName(i));
                    }
                }
            } else if (debouncedState == false && isBtnHeld1[i]) { // Button Held
                if (curT - btnHoldStartT1[i] > REPEAT_INIT_DELAY && curT - lastRepeatT1[i] > REPEAT_INT) {
                    relevantScrollButton = false; // Re-check for auto-repeat
                    scrollDirection = 0;
                     if (currentMenu == MAIN_MENU) {
                        if (i == NAV_UP) { scrollDirection = -1; relevantScrollButton = true; }
                        else if (i == NAV_DOWN) { scrollDirection = 1; relevantScrollButton = true; }
                    } else if (isCarouselActive) {
                        if (i == NAV_LEFT) { scrollDirection = -1; relevantScrollButton = true; }
                        else if (i == NAV_RIGHT) { scrollDirection = 1; relevantScrollButton = true; }
                    } else if (isGridActive) {
                        if (i == NAV_UP) { scrollDirection = -gridCols; relevantScrollButton = true; }
                        else if (i == NAV_DOWN) { scrollDirection = gridCols; relevantScrollButton = true; }
                        else if (i == NAV_LEFT) { scrollDirection = -1; relevantScrollButton = true; }
                        else if (i == NAV_RIGHT) { scrollDirection = 1; relevantScrollButton = true; }
                    }

                    if (relevantScrollButton) {
                        scrollAct(scrollDirection, isGridActive, isCarouselActive);
                        lastRepeatT1[i] = curT; // Update last repeat time
                        // Serial.printf("PCF1 Btn %d (%s) Auto-Repeat\n", i, getPCF1BtnName(i));
                    }
                }
            }
        }
    }
}


void scrollAct(int direction, bool isGridContext, bool isCarouselContext) {
  int oldMenuIndex = menuIndex;

  if (isGridContext) {
    int currentRow = oldMenuIndex / gridCols;
    int currentCol = oldMenuIndex % gridCols;
    int newIndex = oldMenuIndex;
    int numRows = (maxMenuItems + gridCols - 1) / gridCols;

    if (direction == -1) { // NAV_LEFT or Encoder Previous
      if (currentCol == 0) { 
        newIndex = currentRow * gridCols -1;
        if (newIndex < 0 || currentRow == 0) newIndex = maxMenuItems - 1;
      } else {
        newIndex = oldMenuIndex - 1;
      }
    } else if (direction == 1) { // NAV_RIGHT or Encoder Next
      if (currentCol == gridCols - 1 || oldMenuIndex == maxMenuItems - 1) { 
        newIndex = currentRow * gridCols + gridCols; 
        if (newIndex >= maxMenuItems) newIndex = 0; 
      } else {
        newIndex = oldMenuIndex + 1;
      }
    } else if (direction == -gridCols) {  // NAV_UP (direction is negative of gridCols)
      newIndex = oldMenuIndex - gridCols;
      if (newIndex < 0) { 
        int targetCol = currentCol -1;
        if (targetCol < 0) targetCol = gridCols - 1; 

        newIndex = (numRows - 1) * gridCols + targetCol; 
        while(newIndex >= maxMenuItems && newIndex >=0) {
            newIndex -= gridCols; 
        }
        if (newIndex < 0 || newIndex >= maxMenuItems || (newIndex % gridCols != targetCol && maxMenuItems > 0) ) { 
            newIndex = maxMenuItems-1; 
            if (currentCol == 0) newIndex = maxMenuItems-1;
            else { 
                int prevColLastItem = -1;
                for(int k=maxMenuItems-1; k>=0; --k){
                    if(k%gridCols == targetCol){
                        prevColLastItem = k;
                        break;
                    }
                }
                if(prevColLastItem != -1) newIndex = prevColLastItem;
                else newIndex = maxMenuItems-1; 
            }
        }
      }
    } else if (direction == gridCols) {  // NAV_DOWN (direction is positive gridCols)
      newIndex = oldMenuIndex + gridCols;
      if (newIndex >= maxMenuItems) { 
        int targetCol = currentCol + 1;
        if (targetCol >= gridCols) targetCol = 0; 
        newIndex = targetCol; 
        if (newIndex >= maxMenuItems && maxMenuItems > 0) newIndex = 0; 
        else if (maxMenuItems == 0) newIndex = 0;
      }
    }
    menuIndex = newIndex;

    if (maxMenuItems > 0) {
        if (menuIndex >= maxMenuItems) menuIndex = maxMenuItems - 1;
        if (menuIndex < 0) menuIndex = 0;
    } else {
        menuIndex = 0; 
    }

    int currentSelectionRow = menuIndex / gridCols;
    int itemTotalHeight = GRID_ITEM_H + GRID_ITEM_PADDING_Y;
    int gridVisibleAreaY_start_on_screen = STATUS_BAR_H + 1 + GRID_ITEM_PADDING_Y;
    int gridVisibleAreaH_on_screen = 64 - gridVisibleAreaY_start_on_screen - GRID_ITEM_PADDING_Y;
    int visibleRowsOnScreen = gridVisibleAreaH_on_screen > 0 ? gridVisibleAreaH_on_screen / itemTotalHeight : 0;
    if (visibleRowsOnScreen <= 0) visibleRowsOnScreen = 1;
    int screenTopRow = targetGridScrollOffset_Y / itemTotalHeight;

    if (currentSelectionRow < screenTopRow) {
      targetGridScrollOffset_Y = currentSelectionRow * itemTotalHeight;
    } else if (currentSelectionRow >= screenTopRow + visibleRowsOnScreen) {
      targetGridScrollOffset_Y = (currentSelectionRow - visibleRowsOnScreen + 1) * itemTotalHeight;
    }
    if (targetGridScrollOffset_Y < 0) targetGridScrollOffset_Y = 0;
    int totalGridRows = (maxMenuItems + gridCols - 1) / gridCols;
    int maxOffsetY = 0;
    if (totalGridRows > visibleRowsOnScreen && visibleRowsOnScreen > 0) {
        maxOffsetY = (totalGridRows - visibleRowsOnScreen) * itemTotalHeight;
    }
    if (targetGridScrollOffset_Y > maxOffsetY) targetGridScrollOffset_Y = maxOffsetY;

  } else {  // Main Menu or Carousel
    menuIndex += direction;
    if (maxMenuItems > 0) {
        if (menuIndex >= maxMenuItems) menuIndex = 0;
        else if (menuIndex < 0) menuIndex = maxMenuItems - 1;
    } else {
        menuIndex = 0;
    }
  }

  if (oldMenuIndex != menuIndex || (isGridContext && direction != 0)) {
    // Need access to mainMenuAnim and subMenuAnim. These should be extern.
    // Assuming they are declared extern in config.h and defined in KivaMain.ino
    if (currentMenu == MAIN_MENU) {
      mainMenuAnim.setTargets(menuIndex, maxMenuItems);
    } else if (isCarouselContext) {
      subMenuAnim.setTargets(menuIndex, maxMenuItems);
      marqueeActive = false;
      marqueeOffset = 0;
      marqueeScrollLeft = true;
    } else if (isGridContext) {
      marqueeActive = false;
      marqueeOffset = 0;
      marqueeScrollLeft = true;
    }
  }
}

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