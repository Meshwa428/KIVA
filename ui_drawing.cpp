#include "ui_drawing.h"
#include "battery_monitor.h" // For getSmoothV, batPerc
#include "menu_logic.h"      // For menu item text arrays
#include "pcf_utils.h"       // For selectMux needed by drawUI
#include <Arduino.h> // For millis() for uptime clock example

// Externs for Wi-Fi data (defined in KivaMain.ino)
extern WifiNetwork scannedNetworks[MAX_WIFI_NETWORKS];
extern int foundWifiNetworksCount;
extern int wifiMenuIndex; // Used to highlight selected Wi-Fi network


// Animation struct methods
void VerticalListAnimation::init() {
    for (int i = 0; i < MAX_ANIM_ITEMS; i++) {
        itemOffsetY[i] = 0; itemScale[i] = 0.0f;
        targetOffsetY[i] = 0; targetScale[i] = 0.0f;
    }
}
void VerticalListAnimation::setTargets(int selIdx, int total) {
    for (int i = 0; i < MAX_ANIM_ITEMS; i++) {
        if (i < total) {
            int rP = i - selIdx;
            targetOffsetY[i] = rP * itmSpc;
            if (i == selIdx) targetScale[i] = 1.3f;
            else if (abs(rP) == 1) targetScale[i] = 1.f;
            else targetScale[i] = 0.8f;
        } else {
            targetOffsetY[i] = (i - selIdx) * itmSpc; targetScale[i] = 0.f;
        }
    }
}
bool VerticalListAnimation::update() {
    bool anim = false;
    for (int i = 0; i < MAX_ANIM_ITEMS; i++) {
        float oD = targetOffsetY[i] - itemOffsetY[i];
        if (abs(oD) > 0.01f) { itemOffsetY[i] += oD * animSpd * frmTime; anim = true; }
        else { itemOffsetY[i] = targetOffsetY[i]; }
        float sD = targetScale[i] - itemScale[i];
        if (abs(sD) > 0.001f) { itemScale[i] += sD * animSpd * frmTime; anim = true; }
        else { itemScale[i] = targetScale[i]; }
    }
    return anim;
}

void CarouselAnimation::init() {
    for (int i = 0; i < MAX_ANIM_ITEMS; i++) {
        itemOffsetX[i] = 0; itemScale[i] = 0.f;
        targetOffsetX[i] = 0; targetScale[i] = 0.f;
    }
}
void CarouselAnimation::setTargets(int selIdx, int total) {
    for (int i = 0; i < MAX_ANIM_ITEMS; i++) {
        if (i < total) {
            int rP = i - selIdx;
            targetOffsetX[i] = rP * (cardBaseW * 0.8f + cardGap);
            if (rP == 0) targetScale[i] = 1.f;
            else if (abs(rP) == 1) targetScale[i] = 0.75f;
            else if (abs(rP) == 2) targetScale[i] = 0.5f;
            else targetScale[i] = 0.f;
        } else {
            targetScale[i] = 0.f;
            targetOffsetX[i] = (i - selIdx) * (cardBaseW * 0.8f + cardGap);
        }
    }
}
bool CarouselAnimation::update() {
    bool anim = false;
    for (int i = 0; i < MAX_ANIM_ITEMS; i++) {
        float oD = targetOffsetX[i] - itemOffsetX[i];
        if (abs(oD) > 0.1f) { itemOffsetX[i] += oD * animSpd * frmTime; anim = true; }
        else { itemOffsetX[i] = targetOffsetX[i]; }
        float sD = targetScale[i] - itemScale[i];
        if (abs(sD) > 0.01f) { itemScale[i] += sD * animSpd * frmTime; anim = true; }
        else { itemScale[i] = targetScale[i]; }
    }
    return anim;
}


void drawWifiSignalStrength(int x, int y_icon_top, int8_t rssi) {
    // Caller is responsible for u8g2.setDrawColor().
    // x: top-left X of the icon area
    // y_icon_top: top-left Y of the icon area (assuming an 8px high icon area)

    int num_bars_to_draw = 0;

    // --- Calculate num_bars_to_draw based on RSSI ---
    // **IMPORTANT**: Ensure your thresholds here are correct based on actual RSSI values you debug!
    // Example thresholds (adjust these after checking Serial output for raw RSSI):
    if (rssi >= -55)      num_bars_to_draw = 4; 
    else if (rssi >= -80) num_bars_to_draw = 3; 
    else if (rssi >= -90) num_bars_to_draw = 2; 
    else if (rssi >= -100) num_bars_to_draw = 1; 
    // else num_bars_to_draw = 0; // No bars will be drawn if very weak

    // --- Bar geometry ---
    int bar_width = 2;
    int bar_spacing = 1;
    int first_bar_height = 2;       // Height of the shortest bar (bar 0)
    int bar_height_increment = 2;   // Each subsequent bar is taller by this amount
    int max_icon_height = first_bar_height + (4 - 1) * bar_height_increment; // e.g., 2 + 3*2 = 8px

    // --- Render ONLY the number of bars indicated by num_bars_to_draw ---
    for (int i = 0; i < num_bars_to_draw; i++) { // Loop ONLY up to num_bars_to_draw
        int current_bar_height = first_bar_height + i * bar_height_increment;
        
        // Calculate X position for the current bar
        int bar_x_position = x + i * (bar_width + bar_spacing);
        
        // Calculate Y position for the top of the current bar
        int y_pos_for_drawing_this_bar = y_icon_top + (max_icon_height - current_bar_height);

        // Draw this bar as a filled box
        u8g2.drawBox(bar_x_position, y_pos_for_drawing_this_bar, bar_width, current_bar_height);
    }
    // No need to draw empty frames for the remaining bar positions.
}


void drawWifiSetupScreen() {
    const int list_start_y_abs = STATUS_BAR_H + 1;    
    const int list_visible_height = u8g2.getDisplayHeight() - list_start_y_abs; 
    const int list_center_y_on_screen = list_start_y_abs + list_visible_height / 2; 

    const int item_row_h = WIFI_LIST_ITEM_H; 
    const int item_padding_x = 2; 
    const int content_padding_x = 4;

    u8g2.setClipWindow(0, list_start_y_abs, u8g2.getDisplayWidth(), u8g2.getDisplayHeight());
    
    float target_scroll_for_centering = (wifiMenuIndex * item_row_h) - (list_visible_height / 2) + (item_row_h / 2);
    if (maxMenuItems * item_row_h <= list_visible_height) { 
        target_scroll_for_centering = 0; 
    } else {
        if (target_scroll_for_centering < 0) target_scroll_for_centering = 0;
        float max_scroll = (maxMenuItems * item_row_h) - list_visible_height;
        if (max_scroll < 0) max_scroll = 0;
        if (target_scroll_for_centering > max_scroll) target_scroll_for_centering = max_scroll;
    }

    float scrollDiff = target_scroll_for_centering - currentWifiListScrollOffset_Y_anim;
    if (abs(scrollDiff) > 0.1f) {
        currentWifiListScrollOffset_Y_anim += scrollDiff * GRID_ANIM_SPEED * 0.016f; 
        if ((scrollDiff > 0 && currentWifiListScrollOffset_Y_anim > target_scroll_for_centering) ||
            (scrollDiff < 0 && currentWifiListScrollOffset_Y_anim < target_scroll_for_centering)) {
            currentWifiListScrollOffset_Y_anim = target_scroll_for_centering;
        }
    } else {
        currentWifiListScrollOffset_Y_anim = target_scroll_for_centering;
    }

    if (wifiIsScanning) {
        u8g2.setFont(u8g2_font_6x10_tf); 
        u8g2.setDrawColor(1); 
        const char* scanMsg = "Scanning...";
        int msgWidth = u8g2.getStrWidth(scanMsg);
        int text_y_pos = list_center_y_on_screen - (u8g2.getAscent() - u8g2.getDescent())/2 + u8g2.getAscent();
        u8g2.drawStr((u8g2.getDisplayWidth() - msgWidth) / 2, text_y_pos, scanMsg);
    } else { 
        wifiListAnim.update(); 
        u8g2.setFont(u8g2_font_6x10_tf);

        for (int i = 0; i < maxMenuItems; i++) { 
            int item_center_y_in_full_list = (i * item_row_h) + (item_row_h / 2);
            int item_center_on_screen_y = list_start_y_abs + item_center_y_in_full_list - (int)currentWifiListScrollOffset_Y_anim;
            int item_top_on_screen_y = item_center_on_screen_y - item_row_h / 2;
            
            float item_visual_scale = 1.0f;
            if (i < MAX_ANIM_ITEMS && i == wifiMenuIndex) {
                 item_visual_scale = wifiListAnim.itemScale[i]; 
                 if (item_visual_scale < 1.0f) item_visual_scale = 1.0f;
            }

            if (item_top_on_screen_y + item_row_h < list_start_y_abs || item_top_on_screen_y >= u8g2.getDisplayHeight()) {
                continue;
            }
            
            bool is_selected_item = (i == wifiMenuIndex); 
            
            if (is_selected_item) {
                u8g2.setDrawColor(1); 
                drawRndBox(item_padding_x, item_top_on_screen_y, 
                           u8g2.getDisplayWidth() - 2 * item_padding_x, item_row_h, 
                           2, true);
                u8g2.setDrawColor(0); 
            } else {
                u8g2.setDrawColor(1); 
            }
            
            const char* currentItemText = NULL; 
            bool isNetworkItem = false;
            int8_t currentRssi = 0;
            bool currentIsSecure = false;
            int action_icon_type = -1;

            if (i < foundWifiNetworksCount) { 
                currentItemText = scannedNetworks[i].ssid;
                currentRssi = scannedNetworks[i].rssi;
                currentIsSecure = scannedNetworks[i].isSecure;
                isNetworkItem = true;
            } else if (i == foundWifiNetworksCount) { 
                currentItemText = "Scan Again";
                action_icon_type = 16; 
            } else if (i == foundWifiNetworksCount + 1) { 
                currentItemText = "Back";
                action_icon_type = 8; 
            }

            if (currentItemText == NULL || currentItemText[0] == '\0') continue;
            
            int current_content_x = content_padding_x + item_padding_x; 
            int icon_y_center_for_draw = item_center_on_screen_y; 

            if (isNetworkItem) {
                // Signal strength is drawn with the current draw color (black on selected, white on unselected)
                drawWifiSignalStrength(current_content_x, icon_y_center_for_draw - 4, currentRssi); // -4 for 8px high icon
                current_content_x += 12; // Width for signal icon (4 bars * 2px + 3 spaces = 11) + 1px gap

                if(currentIsSecure) {
                    // Lock Icon Attempt 3 (direct draw, ~5px wide body, shackle on top)
                    int lock_body_start_x = current_content_x;
                    // Aim to center the whole lock (body + shackle) vertically around icon_y_center_for_draw
                    // Body height: 4px. Shackle height: ~3px. Total ~7px.
                    // Top of shackle: icon_y_center_for_draw - 3 (approx)
                    // Top of body: icon_y_center_for_draw (approx)
                    // Bottom of body: icon_y_center_for_draw + 3 (approx)
                    
                    int body_top_y = icon_y_center_for_draw - 1; // Let body be slightly above center
                    int body_width = 5;
                    int body_height = 4;

                    // Draw Lock Body
                    u8g2.drawBox(lock_body_start_x, body_top_y, body_width, body_height);

                    // Draw Shackle (aiming for rounder top)
                    int shackle_top_most_y = body_top_y - 3; // 3 pixels above the body

                    // Top "flat" part of the shackle (2 pixels wide)
                    u8g2.drawHLine(lock_body_start_x + 1, shackle_top_most_y, 3); // Draw from x+1 to x+3 (3px wide flat top)

                    // Upper "curved" shoulders of the shackle
                    u8g2.drawPixel(lock_body_start_x, shackle_top_most_y + 1);     // Left shoulder
                    u8g2.drawPixel(lock_body_start_x + 4, shackle_top_most_y + 1); // Right shoulder
                    
                    // Vertical sides of the shackle connecting to the body
                    u8g2.drawVLine(lock_body_start_x, shackle_top_most_y + 2, 1);     // Left side connection
                    u8g2.drawVLine(lock_body_start_x + 4, shackle_top_most_y + 2, 1); // Right side connection
                    
                    current_content_x += 7; // Approx width for lock icon (5px) + 2px gap
                } else {
                    current_content_x += 7; // Keep spacing consistent if no lock
                }
            } else { // Action items ("Scan Again", "Back")
                if (action_icon_type != -1) {
                    // Icons for action items also need to respect selected/unselected color
                    drawCustomIcon(current_content_x, icon_y_center_for_draw - 4, action_icon_type, false); // small icon
                    current_content_x += 10; 
                } else {
                     current_content_x += 10; // Placeholder if no icon
                }
            }

            int text_x_start = current_content_x; 
            int text_available_width = u8g2.getDisplayWidth() - text_x_start - content_padding_x - item_padding_x;
            int text_baseline_y = item_center_on_screen_y - (u8g2.getAscent() + u8g2.getDescent())/2 + u8g2.getAscent();
            
            if (is_selected_item && isNetworkItem) { 
                updateMarquee(text_available_width, currentItemText);
                if (marqueeActive) {
                    u8g2.drawStr(text_x_start + (int)marqueeOffset, text_baseline_y, marqueeText);
                } else {
                    char* truncated = truncateText(currentItemText, text_available_width, u8g2);
                    u8g2.drawStr(text_x_start, text_baseline_y, truncated);
                }
            } else { 
                char* truncated = truncateText(currentItemText, text_available_width, u8g2);
                u8g2.drawStr(text_x_start, text_baseline_y, truncated);
            }
        }
    }
    u8g2.setDrawColor(1); // Reset draw color to white for subsequent draws outside this function
    u8g2.setMaxClipWindow(); 
}

void drawUI() {
  unsigned long currentTime = millis(); 
  
  // --- Draw Passive Display (CH2) ---
  selectMux(MUX_CHANNEL_SECOND_DISPLAY);
  drawPassiveDisplay(); // Call the new function

  // --- Draw Main Display (CH4) ---
  selectMux(MUX_CHANNEL_MAIN_DISPLAY); 
  u8g2.clearBuffer();
  drawStatusBar(); 

  if (currentMenu == TOOL_CATEGORY_GRID) {
    // ... (grid animation and drawing logic for u8g2 - main display)
    float scrollDiff = targetGridScrollOffset_Y - currentGridScrollOffset_Y_anim;
    if (abs(scrollDiff) > 0.1f) {
      currentGridScrollOffset_Y_anim += scrollDiff * GRID_ANIM_SPEED * 0.016f;
      if ((scrollDiff > 0 && currentGridScrollOffset_Y_anim > targetGridScrollOffset_Y) ||
          (scrollDiff < 0 && currentGridScrollOffset_Y_anim < targetGridScrollOffset_Y)) {
        currentGridScrollOffset_Y_anim = targetGridScrollOffset_Y;
      }
    } else {
      currentGridScrollOffset_Y_anim = targetGridScrollOffset_Y;
    }

    if (gridAnimatingIn) {
      bool stillAnimating = false;
      int currentGridItemCount = maxMenuItems; 
      for (int i = 0; i < currentGridItemCount && i < MAX_GRID_ITEMS; ++i) {
        if (currentTime >= gridItemAnimStartTime[i]) {
          float targetScaleForThisItem = gridItemTargetScale[i];
          float diff = targetScaleForThisItem - gridItemScale[i];
          if (abs(diff) > 0.01f) {
            gridItemScale[i] += diff * GRID_ANIM_SPEED * 0.016f;
            if ((diff > 0 && gridItemScale[i] > targetScaleForThisItem) || 
                (diff < 0 && gridItemScale[i] < targetScaleForThisItem)) {
              gridItemScale[i] = targetScaleForThisItem;
            }
            stillAnimating = true;
          }
        } else { 
          stillAnimating = true; 
        }
      }
      if (!stillAnimating) gridAnimatingIn = false;
    } else { 
      for (int i = 0; i < maxMenuItems && i < MAX_GRID_ITEMS; ++i) {
        gridItemScale[i] = 1.0f;
      }
      for (int i = maxMenuItems; i < MAX_GRID_ITEMS; ++i) { 
        gridItemScale[i] = 0.0f;
      }
    }
    drawToolGridScreen(); // Draws on u8g2 (main display)
  } else if (currentMenu == MAIN_MENU) {
    drawMainMenu(); // Draws on u8g2 (main display)
  } else if (currentMenu == WIFI_SETUP_MENU) { // <--- ADDED CASE
    drawWifiSetupScreen();
  } else { 
    drawCarouselMenu(); // Draws on u8g2 (main display)
  }
  u8g2.sendBuffer(); // Send main display buffer
}

// Modify drawPassiveDisplay for correct context title
void drawPassiveDisplay() {
    u8g2_small.clearBuffer();
    u8g2_small.setFont(u8g2_font_5x7_tf); 

    u8g2_small.drawStr(0, 7, "W: ---"); 
    u8g2_small.drawStr(u8g2_small.getDisplayWidth() / 2, 7, "B: ---");

    unsigned long now = millis();
    unsigned long allSeconds = now / 1000;
    int runHours = allSeconds / 3600;
    int secsRemaining = allSeconds % 3600;
    int runMinutes = secsRemaining / 60;
    int runSeconds = secsRemaining % 60;
    char timeStr[9]; 
    sprintf(timeStr, "%02d:%02d:%02d", runHours, runMinutes, runSeconds);
    
    u8g2_small.setFont(u8g2_font_helvB08_tf); 
    int timeStrWidth = u8g2_small.getStrWidth(timeStr);
    u8g2_small.drawStr((u8g2_small.getDisplayWidth() - timeStrWidth) / 2, 19, timeStr); 

    u8g2_small.setFont(u8g2_font_5x7_tf); 
    uint8_t battPercentage = batPerc(getSmoothV());
    char battStr[12];
    sprintf(battStr, "[");
    int bars = battPercentage / 25; 
    for(int b=0; b<4; ++b) {
        if(b < bars) strcat(battStr, "|");
        else strcat(battStr, " ");
    }
    strcat(battStr, "]");
    sprintf(battStr + strlen(battStr), " %d%%", battPercentage); 
    u8g2_small.drawStr(0, 31, battStr);

    // --- Corrected Mode Text Logic ---
    const char* modeText = "---";
    if (currentMenu == MAIN_MENU && menuIndex < getMainMenuItemsCount()) {
         modeText = mainMenuItems[menuIndex]; // Show selected main menu item
    } else if (currentMenu == TOOLS_MENU) { // Carousel active
         modeText = "Tools"; // Show the title of the carousel
    } else if (currentMenu == GAMES_MENU) { // Carousel active
         modeText = "Games"; // Show the title of the carousel
    } else if (currentMenu == SETTINGS_MENU) { // Carousel active
         modeText = "Settings"; // Show the title of the carousel
    } else if (currentMenu == TOOL_CATEGORY_GRID && toolsCategoryIndex < (getToolsMenuItemsCount() -1) ) {
         modeText = toolsMenuItems[toolsCategoryIndex]; // Show the category name from Tools Menu
    } else if (currentMenu == WIFI_SETUP_MENU) {
        modeText = "Wi-Fi";
    }
    // Add other specific screen titles here if needed

    char truncatedModeText[10]; 
    strncpy(truncatedModeText, modeText, sizeof(truncatedModeText)-1);
    truncatedModeText[sizeof(truncatedModeText)-1] = '\0';

    // Use a temporary buffer for truncateText if SBUF might be used elsewhere concurrently (not an issue here)
    char tempTruncBuf[10]; // Must be SBUF's size or handled carefully
    strcpy(tempTruncBuf, truncateText(modeText, 38, u8g2_small)); // Max width for mode text area
                                                                  // Pass u8g2_small for correct font metrics
    
    int modeTextWidth = u8g2_small.getStrWidth(tempTruncBuf);
    u8g2_small.drawStr(u8g2_small.getDisplayWidth() - modeTextWidth - 1, 31, tempTruncBuf); // Right aligned a bit

    u8g2_small.sendBuffer();
}



void updateMarquee(int cardInnerW, const char* text) {
  if (text == nullptr || strlen(text) == 0) {
    marqueeActive = false;
    marqueeText[0] = '\0'; 
    return;
  }
  // Font should be set by the caller context (drawCarouselMenu or drawToolGridScreen)
  // before calling u8g2.getUTF8Width or drawing.
  // For getUTF8Width here, we assume the caller set the correct font.

  bool shouldBeActive = u8g2.getUTF8Width(text) > cardInnerW;

  if (!marqueeActive && shouldBeActive) {
    strncpy(marqueeText, text, sizeof(marqueeText) - 1);
    marqueeText[sizeof(marqueeText) - 1] = '\0';
    marqueeTextLenPx = u8g2.getUTF8Width(marqueeText); // Uses current u8g2 font
    marqueeOffset = 0;
    marqueeActive = true;
    marqueePaused = true;
    marqueeScrollLeft = true;
    marqueePauseStartTime = millis();
    lastMarqueeTime = millis();
  } else if (marqueeActive && !shouldBeActive) {
    marqueeActive = false;
    marqueeText[0] = '\0';
  } else if (marqueeActive && shouldBeActive) {
    if (strncmp(marqueeText, text, sizeof(marqueeText) -1) != 0) {
        strncpy(marqueeText, text, sizeof(marqueeText) - 1);
        marqueeText[sizeof(marqueeText) - 1] = '\0';
        marqueeTextLenPx = u8g2.getUTF8Width(marqueeText); // Uses current u8g2 font
        marqueeOffset = 0;
        marqueePaused = true;
        marqueeScrollLeft = true;
        marqueePauseStartTime = millis();
        lastMarqueeTime = millis();
    }
  }

  if (!marqueeActive) return;

  unsigned long curT = millis();
  if (marqueePaused) {
    if (curT - marqueePauseStartTime > MARQUEE_PAUSE_DURATION) {
      marqueePaused = false;
      lastMarqueeTime = curT;
    }
    return;
  }

  if (curT - lastMarqueeTime > MARQUEE_UPDATE_INTERVAL) { // Used #define from config.h
    if (marqueeScrollLeft) {
      marqueeOffset -= MARQUEE_SCROLL_SPEED; // Used #define from config.h
    } else {
      marqueeOffset += MARQUEE_SCROLL_SPEED; // Used #define from config.h
    }
    lastMarqueeTime = curT;

    if (marqueeTextLenPx <= cardInnerW) { 
        marqueeActive = false;
        marqueeText[0] = '\0';
        return;
    }

    if (marqueeScrollLeft && marqueeOffset <= -(marqueeTextLenPx - cardInnerW)) { 
      marqueeOffset = -(marqueeTextLenPx - cardInnerW);
      marqueePaused = true;
      marqueePauseStartTime = curT;
      marqueeScrollLeft = false;
    } else if (!marqueeScrollLeft && marqueeOffset >= 0) {
      marqueeOffset = 0;
      marqueePaused = true;
      marqueePauseStartTime = curT;
      marqueeScrollLeft = true;
    }
  }
}

char* truncateText(const char* originalText, int maxWidthPixels, U8G2 &display) {
  // SBUF is extern char SBUF[32]; defined in KivaMain.ino, declared in config.h
  int len = strlen(originalText);
  if (len == 0) {
    SBUF[0] = '\0';
    return SBUF;
  }
  strncpy(SBUF, originalText, sizeof(SBUF) - 1);
  SBUF[sizeof(SBUF) - 1] = '\0';

  if (display.getUTF8Width(SBUF) <= maxWidthPixels) return SBUF;

  if (strlen(SBUF) < 3) { 
      while(display.getUTF8Width(SBUF) > maxWidthPixels && strlen(SBUF) > 0) {
          SBUF[strlen(SBUF)-1] = '\0';
      }
      return SBUF;
  }
  
  // Place "..." at the end, ensure enough space by shortening first
  // Example: "LongText" -> "Long..."
  // Max length for SBUF is 31 + null. "..." is 3 chars. So text can be up to 28.
  int sbufMaxTextLen = sizeof(SBUF) - 1 - 3; // -1 for null, -3 for "..."
  if (len > sbufMaxTextLen) {
      SBUF[sbufMaxTextLen] = '\0'; // Truncate SBUF to make space for "..."
  }
  strcat(SBUF, "..."); // This might now go past sizeof(SBUF) if not careful above.
                      // Corrected:
  strncpy(SBUF, originalText, sbufMaxTextLen); // Copy max possible text part
  SBUF[sbufMaxTextLen] = '\0'; // Null terminate text part
  strcat(SBUF, "...");         // Append "..."

  while (display.getUTF8Width(SBUF) > maxWidthPixels && strlen(SBUF) > 3) {
    SBUF[strlen(SBUF) - 4] = '\0'; 
    strcat(SBUF, "...");
  }

  if (display.getUTF8Width(SBUF) > maxWidthPixels) { 
    if (maxWidthPixels >= display.getUTF8Width("..")) {
        strcpy(SBUF, "..");
        if (display.getUTF8Width(SBUF) > maxWidthPixels && maxWidthPixels >= display.getUTF8Width(".")) {
            strcpy(SBUF, ".");
        } else if (display.getUTF8Width(SBUF) > maxWidthPixels) {
            SBUF[0] = '\0';
        }
    } else if (maxWidthPixels >= display.getUTF8Width(".")) {
        strcpy(SBUF, ".");
         if (display.getUTF8Width(SBUF) > maxWidthPixels) {
            SBUF[0] = '\0';
        }
    } else {
        SBUF[0] = '\0';
    }
  }
  return SBUF;
}

// In drawStatusBar() in ui_drawing.cpp
void drawStatusBar() {
    float v = getSmoothV(); 
    uint8_t s = batPerc(v);
    u8g2.setFont(u8g2_font_6x10_tf); // Default font for status bar text
    const char* titleText = "Kiva OS"; 

    // --- Title Logic (remains the same) ---
    if (currentMenu == TOOL_CATEGORY_GRID) { 
        if (toolsCategoryIndex >= 0 && toolsCategoryIndex < (getToolsMenuItemsCount() -1) ) { 
            titleText = toolsMenuItems[toolsCategoryIndex];
        } else {
            titleText = "Tools"; 
        }
    } else if (currentMenu == WIFI_SETUP_MENU) { 
        titleText = "Wi-Fi Setup";
    }
    else if (currentMenu != MAIN_MENU && mainMenuSavedIndex >= 0 && mainMenuSavedIndex < getMainMenuItemsCount()) { 
        titleText = mainMenuItems[mainMenuSavedIndex];
    }

    char titleBuffer[14]; 
    strncpy(titleBuffer, titleText, sizeof(titleBuffer) - 1);
    titleBuffer[sizeof(titleBuffer) - 1] = '\0';
    int max_title_width = 65; // Max width for title to leave space for battery info
     if (u8g2.getStrWidth(titleBuffer) > max_title_width) { 
        truncateText(titleText, max_title_width - 5, u8g2); // Truncate a bit more to ensure "..." fits
        strncpy(titleBuffer, SBUF, sizeof(titleBuffer)-1); 
        titleBuffer[sizeof(titleBuffer) - 1] = '\0';
    }
    u8g2.drawStr(2, 7, titleBuffer); // y=7 for 6x10 font baseline

    // --- Battery Display Area (Right Aligned) ---
    int battery_area_right_margin = 2;
    int current_x_origin_for_battery = u8g2.getDisplayWidth() - battery_area_right_margin;

    // 1. Draw Battery Icon (Simplified)
    // Icon is 9px wide, 5px high. Status bar top is y=0.
    // Status bar height is 11. Icon height 5. y_top = (11-5)/2 = 3.
    int bat_icon_y_top = 3; 
    int bat_icon_width = 9; 
    current_x_origin_for_battery -= bat_icon_width; 
    drawBatIcon(current_x_origin_for_battery, bat_icon_y_top, s);

    // 2. Draw Charging Icon (if charging) - Using a Glyph
    if (isCharging) {
        // The 'zap' (lightning bolt) glyph from open_iconic_all_1x is 0x00BA (decimal 186)
        // or 0xE00D in some mappings. Check u8g2 docs for u8g2_font_open_iconic_all_1x_t
        // For u8g2_font_open_iconic_embedded_1x_t, 'P' (0x50) can look like a plug, or zap is often 0xBA.
        // Let's try a common lightning bolt glyph ID for icon fonts.
        // Glyphs are typically 8x8.
        int charge_icon_glyph_width = 8; // Approximate width of the glyph rendering area
        int charge_icon_spacing = 1;
        current_x_origin_for_battery -= (charge_icon_glyph_width + charge_icon_spacing);

        u8g2.setFont(u8g2_font_open_iconic_all_1x_t); // Switch to icon font
        // Glyph y position needs to be baseline. For an 8x8 icon font, if status bar font baseline is 7,
        // and icon font ascent is ~7, its baseline could also be 7.
        // Or center its 8px height: (11-8)/2 = 1.5 (y_top). Baseline y_top + ascent.
        // Ascent for open_iconic_all_1x_t is often 7 or 8.
        // If status bar font baseline is 7, and icon font ascent is 7, glyph_y = 7.
        int glyph_y_baseline = 7; 
        // Try glyph U+26A1 (HIGH VOLTAGE SIGN ☇ ) or find one in open_iconic like 0xBA
        // u8g2 often uses different ranges. 0x7A in open_iconic_weather_1x is lightning.
        // For open_iconic_all_1x_t, zap/lightning might be 0xBA or similar.
        // For this example, let's use the 'flash' / 'zap' icon often mapped to 0xBA or 0xE00D.
        // Check the u8g2 font guide for your specific version.
        // Common one is 'flash' icon (lightning bolt):
        // In u8g2_font_open_iconic_all_1x_t, 0xE00D is often 'bolt'. Or 0xBA.
        // Let's assume 0xE00D based on some common u8g2 mappings for `pf` fonts converted to `u8g2`.
        // The `u8x8_GetGlyphBitmap` tool from u8g2 can help find codes.
        // Alternatively, `flash` in `u8g2_font_streamline_interface_essential_16.tf` is good but too big.
        // The character '!' (0x21) or '$' (0x24) might be an abstract charging symbol if a bolt isn't easily found.
        // For a test, let's use '!'
        // u8g2.drawGlyph(current_x_origin_for_battery, glyph_y_baseline, 0x21); // '!'
        // A better option from open_iconic_all_1x_t could be 0xBA (often a lightning/zap)
         u8g2.drawGlyph(current_x_origin_for_battery + 1, glyph_y_baseline, 0xBA); // Try 0xBA, shift by 1 for centering 6px wide glyph

        u8g2.setFont(u8g2_font_6x10_tf); // Switch back to default text font
    }

    // 3. Draw Battery Percentage Text
    char batteryStr[6]; 
    snprintf(batteryStr, sizeof(batteryStr), "%u%%", s);
    int percent_text_width = u8g2.getStrWidth(batteryStr);
    int percent_text_spacing = 2;
    current_x_origin_for_battery -= (percent_text_width + percent_text_spacing); 
    u8g2.drawStr(current_x_origin_for_battery, 7, batteryStr); 

    // --- Status Bar Line ---
    u8g2.setDrawColor(1); 
    u8g2.drawLine(0, STATUS_BAR_H - 1, u8g2.getDisplayWidth() - 1, STATUS_BAR_H - 1); 
}


void drawMainMenu() {
  mainMenuAnim.update();
  const char** itms = mainMenuItems; // From menu_logic.cpp via extern in menu_logic.h
  const int icon_h = 16; 
  const int icon_w = 16; 
  const int icon_padding_right = 4;  
  const int list_start_y = STATUS_BAR_H + 1;
  const int list_visible_h = 64 - list_start_y;
  const int list_center_y_abs = list_start_y + list_visible_h / 2;

  u8g2.setClipWindow(0, list_start_y, 127, 63);

  for (int i = 0; i < maxMenuItems; i++) {
    int item_center_y_relative = (int)mainMenuAnim.itemOffsetY[i];
    float current_scale = mainMenuAnim.itemScale[i];
    if (current_scale <= 0.01f) continue;

    int text_box_base_width = 65; 
    int text_box_base_height = 12; 
    int text_box_w_scaled = (int)(text_box_base_width * current_scale);
    int text_box_h_scaled = (int)(text_box_base_height * current_scale); 

    int item_abs_center_y = list_center_y_abs + item_center_y_relative;
    
    // Effective item height for culling is max of icon or scaled text box
    int effective_item_h = max(icon_h, text_box_h_scaled);
    if (item_abs_center_y - effective_item_h / 2 > 63 || item_abs_center_y + effective_item_h / 2 < list_start_y) {
        continue; 
    }

    int total_visual_width = icon_w + icon_padding_right + text_box_w_scaled;
    int item_start_x_abs = (128 - total_visual_width) / 2;
    if (item_start_x_abs < 2) item_start_x_abs = 2; 

    int icon_x_pos = item_start_x_abs;
    int icon_y_pos = item_abs_center_y - icon_h / 2; 

    int text_box_x_pos = icon_x_pos + icon_w + icon_padding_right;
    int text_box_y_pos = item_abs_center_y - text_box_h_scaled / 2;

    bool is_selected_item = (i == menuIndex);

    u8g2.setDrawColor(1); 
    drawCustomIcon(icon_x_pos, icon_y_pos, i, true); // Main menu uses iconType = index i, large

    u8g2.setFont(u8g2_font_6x10_tf); // Font for main menu text
    int text_width_pixels = u8g2.getStrWidth(itms[i]);
    // Center text inside its scaled text_box
    int text_x_render_pos = text_box_x_pos + (text_box_w_scaled - text_width_pixels) / 2;
    // Approx vertical center for 6x10 font (ascent is ~7-8)
    int text_y_baseline_render_pos = item_abs_center_y + 4; 

    if (text_box_w_scaled > 0 && text_box_h_scaled > 0) {
      if (is_selected_item) {
        u8g2.setDrawColor(1); // White box
        drawRndBox(text_box_x_pos, text_box_y_pos, text_box_w_scaled, text_box_h_scaled, 2, true); // Filled
        u8g2.setDrawColor(0); // Black text
        u8g2.drawStr(text_x_render_pos, text_y_baseline_render_pos, itms[i]);
      } else {
        u8g2.setDrawColor(1); // White frame and text
        drawRndBox(text_box_x_pos, text_box_y_pos, text_box_w_scaled, text_box_h_scaled, 2, false); // Frame only
        u8g2.drawStr(text_x_render_pos, text_y_baseline_render_pos, itms[i]);
      }
    }
  }
  u8g2.setDrawColor(1); // Reset default draw color
  u8g2.setMaxClipWindow();
}


void drawCarouselMenu() {
  subMenuAnim.update();
  const int carousel_center_y_abs = 44; 
  const int screen_center_x_abs = 64; 
  const int card_internal_padding = 3;
  const int icon_render_size = 16; 
  const int icon_margin_top = 3; 
  const int text_margin_from_icon = 2; 

  // Set font for selected item text here, as updateMarquee relies on current font for width
  u8g2.setFont(u8g2_font_6x10_tf); 
  
  for (int i = 0; i < maxMenuItems; i++) {
    float current_item_scale = subMenuAnim.itemScale[i];
    if (current_item_scale <= 0.05f) continue; 

    float current_item_offset_x = subMenuAnim.itemOffsetX[i];
    int card_w_scaled = (int)(CarouselAnimation().cardBaseW * current_item_scale); // Access const from type
    int card_h_scaled = (int)(CarouselAnimation().cardBaseH * current_item_scale);
    int card_x_abs = screen_center_x_abs + (int)current_item_offset_x - (card_w_scaled / 2);
    int card_y_abs = carousel_center_y_abs - (card_h_scaled / 2);
    bool is_selected_item = (i == menuIndex);

    const char** current_item_list_ptr = nullptr;
    int icon_type_for_item = 0; 

    // Determine item list and icon type based on currentMenu
    if (currentMenu == GAMES_MENU) {
      current_item_list_ptr = gamesMenuItems;
      if (i == 0) icon_type_for_item = 4; else if (i == 1) icon_type_for_item = 5; 
      else if (i == 2) icon_type_for_item = 6; else if (i == 3) icon_type_for_item = 7; 
      else if (i == 4) icon_type_for_item = 8; else continue;

    } else if (currentMenu == TOOLS_MENU) {
      current_item_list_ptr = toolsMenuItems;
      if (i == 0) icon_type_for_item = 12; else if (i == 1) icon_type_for_item = 9;  
      else if (i == 2) icon_type_for_item = 10; else if (i == 3) icon_type_for_item = 9;  
      else if (i == 4) icon_type_for_item = 11; else if (i == 5) icon_type_for_item = 8;  
      else continue;

    } else if (currentMenu == SETTINGS_MENU) {
      current_item_list_ptr = settingsMenuItems; 
      // settingsMenuItems[] = { "Wi-Fi Setup", "Display Opts", "Sound Setup", "System Info", "About Kiva", "Back" };
      if (i == 0) icon_type_for_item = 9;   // "Wi-Fi Setup" (Wi-Fi Icon) << NEW
      else if (i == 1) icon_type_for_item = 13;  // "Display Opts" (Monitor Icon)
      else if (i == 2) icon_type_for_item = 14;  // "Sound Setup" (Speaker Icon)
      else if (i == 3) icon_type_for_item = 15;  // "System Info" (Chip Icon)
      else if (i == 4) icon_type_for_item = 3;   // "About Kiva" ('i' in circle Icon)
      else if (i == 5) icon_type_for_item = 8;   // "Back" 
      else continue;

    } else {
      continue; 
    }

    if (card_w_scaled <= 0 || card_h_scaled <= 0) continue;

    if (is_selected_item) {
      u8g2.setDrawColor(1); 
      drawRndBox(card_x_abs, card_y_abs, card_w_scaled, card_h_scaled, 3, true); 

      u8g2.setDrawColor(0); 
      int icon_x_pos = card_x_abs + (card_w_scaled - icon_render_size) / 2;
      int icon_y_pos = card_y_abs + icon_margin_top;
      drawCustomIcon(icon_x_pos, icon_y_pos, icon_type_for_item, true); // Carousel uses large icons

      // Font for selected item text is already u8g2_font_6x10_tf (set before loop)
      int text_area_y_start_abs = card_y_abs + icon_margin_top + icon_render_size + text_margin_from_icon;
      int text_area_h_available = card_h_scaled - (text_area_y_start_abs - card_y_abs) - card_internal_padding;
      // Center baseline in available height
      int text_baseline_y_render = text_area_y_start_abs + (text_area_h_available - (u8g2.getAscent() - u8g2.getDescent())) / 2 + u8g2.getAscent() -1;


      int card_inner_content_width = card_w_scaled - 2 * card_internal_padding;
      updateMarquee(card_inner_content_width, current_item_list_ptr[i]); 

      // Clip window for text rendering inside the card
      int clip_x1 = card_x_abs + card_internal_padding;
      int clip_y1 = text_area_y_start_abs; 
      int clip_x2 = card_x_abs + card_w_scaled - card_internal_padding;
      int clip_y2 = card_y_abs + card_h_scaled - card_internal_padding; 

      clip_y1 = max(clip_y1, card_y_abs); // Ensure clip is within card bounds
      clip_y2 = min(clip_y2, card_y_abs + card_h_scaled);

      if (clip_x1 < clip_x2 && clip_y1 < clip_y2) { // Valid clip area
        u8g2.setClipWindow(clip_x1, clip_y1, clip_x2, clip_y2);
        if (marqueeActive) {
          u8g2.drawStr(card_x_abs + card_internal_padding + (int)marqueeOffset, text_baseline_y_render, marqueeText);
        } else {
          int text_width_pixels = u8g2.getStrWidth(current_item_list_ptr[i]);
          u8g2.drawStr(card_x_abs + (card_w_scaled - text_width_pixels) / 2, text_baseline_y_render, current_item_list_ptr[i]);
        }
        u8g2.setMaxClipWindow(); // Reset clip after drawing text
      }
    } else { // Not selected item
      u8g2.setDrawColor(1); 
      drawRndBox(card_x_abs, card_y_abs, card_w_scaled, card_h_scaled, 2, false); // Frame only

      if (current_item_scale > 0.5) { // Only draw text if item is reasonably scaled
        u8g2.setFont(u8g2_font_5x7_tf); // Smaller font for non-selected items
        char* display_text_truncated = truncateText(current_item_list_ptr[i], card_w_scaled - 2 * card_internal_padding, u8g2);
        int text_width_pixels = u8g2.getStrWidth(display_text_truncated);
        // Center baseline for 5x7 font
        int text_baseline_y_render = card_y_abs + (card_h_scaled - (u8g2.getAscent() - u8g2.getDescent())) / 2 + u8g2.getAscent();
        u8g2.drawStr(card_x_abs + (card_w_scaled - text_width_pixels) / 2, text_baseline_y_render, display_text_truncated);
      }
    }
  }
  u8g2.setDrawColor(1); // Reset default draw color after all items
}


void startGridItemAnimation() {
  gridAnimatingIn = true;
  unsigned long currentTime = millis();
  int currentGridItemCount = maxMenuItems; 

  for (int i = 0; i < MAX_GRID_ITEMS; i++) {
    if (i < currentGridItemCount) {
      gridItemScale[i] = 0.0f; // Start from zero scale
      gridItemTargetScale[i] = 1.0f; // Target full scale
      int row = i / gridCols;
      int col = i % gridCols;
      // Stagger animation: items in later rows/cols start slightly later
      gridItemAnimStartTime[i] = currentTime + (unsigned long)((row * 1.5f + col) * GRID_ANIM_STAGGER_DELAY);
    } else {
      gridItemScale[i] = 0.0f; // Items not in current view (or beyond max)
      gridItemTargetScale[i] = 0.0f; 
      gridItemAnimStartTime[i] = currentTime; // Start their "animation to zero" (no-op if already 0)
    }
  }
}

void drawToolGridScreen() {
  const char** current_tool_list_ptr = nullptr;
  int current_tool_count = maxMenuItems; 

  switch (toolsCategoryIndex) {
    case 0: current_tool_list_ptr = injectionToolItems; break;
    case 1: current_tool_list_ptr = wifiAttackToolItems; break;
    case 2: current_tool_list_ptr = bleAttackToolItems; break;
    case 3: current_tool_list_ptr = nrfReconToolItems; break;
    case 4: current_tool_list_ptr = jammingToolItems; break;
    default: return; // Should not happen
  }

  if (!current_tool_list_ptr) return; // Safety check

  int item_box_base_width = (128 - (gridCols + 1) * GRID_ITEM_PADDING_X) / gridCols;
  const int grid_content_area_start_y_abs = STATUS_BAR_H + 1; 
  const int first_item_row_start_y_abs = grid_content_area_start_y_abs + GRID_ITEM_PADDING_Y; 

  u8g2.setFont(u8g2_font_5x7_tf); // Font for grid item text

  // MASTER CLIP WINDOW for the entire grid drawing area (below status bar)
  u8g2.setClipWindow(0, grid_content_area_start_y_abs, 127, 63);

  for (int i = 0; i < current_tool_count; i++) {
    int row = i / gridCols;
    int col = i % gridCols;

    float current_item_effective_scale = gridAnimatingIn ? gridItemScale[i] : 1.0f;

    int current_item_box_w_scaled = (int)(item_box_base_width * current_item_effective_scale);
    int current_item_box_h_scaled = (int)(GRID_ITEM_H * current_item_effective_scale);

    if (current_item_box_w_scaled < 1 || current_item_box_h_scaled < 1) continue;

    // Absolute X position of the box (not affected by Y scroll)
    int box_x_abs = GRID_ITEM_PADDING_X + col * (item_box_base_width + GRID_ITEM_PADDING_X) + (item_box_base_width - current_item_box_w_scaled) / 2;
    
    // Absolute Y position of the top of the box for this item if grid wasn't scrolled
    int box_y_abs_no_scroll = first_item_row_start_y_abs + row * (GRID_ITEM_H + GRID_ITEM_PADDING_Y) + (GRID_ITEM_H - current_item_box_h_scaled) / 2;
    
    // ON-SCREEN Y position of the top of the box, after applying scroll animation
    int box_y_on_screen_top = box_y_abs_no_scroll - (int)currentGridScrollOffset_Y_anim;

    // Culling: Check if the item is visible within the master clip window
    if (box_y_on_screen_top + current_item_box_h_scaled <= grid_content_area_start_y_abs || box_y_on_screen_top >= 64) {
      continue;
    }

    bool is_selected_item = (i == menuIndex);

    // Draw the box. It will be automatically clipped by the master u8g2.setClipWindow call above.
    drawRndBox(box_x_abs, box_y_on_screen_top, current_item_box_w_scaled, current_item_box_h_scaled, 2, is_selected_item);

    if (current_item_effective_scale > 0.7f) { // Only draw text if scaled enough
      u8g2.setDrawColor(is_selected_item ? 0 : 1); // Black text on white selected, white on black unselected

      // Define the text drawing area relative to the box's on-screen position
      int text_content_x_start = box_x_abs + 2; // Small padding inside the box for text
      // For 5x7 font, ascent is ~5.
      int text_content_y_baseline = box_y_on_screen_top + (current_item_box_h_scaled - u8g2.getAscent()) / 2 + u8g2.getAscent() -1; 
      int text_available_render_width = current_item_box_w_scaled - 4; // width - padding_left - padding_right

      // Child clip window for the text within this specific item's box.
      int text_clip_x1 = max(box_x_abs + 1, 0); 
      int text_clip_y1 = max(box_y_on_screen_top + 1, grid_content_area_start_y_abs); 
      int text_clip_x2 = min(box_x_abs + current_item_box_w_scaled - 1, 127); 
      int text_clip_y2 = min(box_y_on_screen_top + current_item_box_h_scaled - 1, 63); 

      if (text_clip_x1 < text_clip_x2 && text_clip_y1 < text_clip_y2) { // If valid clip area
        u8g2.setClipWindow(text_clip_x1, text_clip_y1, text_clip_x2, text_clip_y2);
        
        if (is_selected_item) { 
          updateMarquee(text_available_render_width, current_tool_list_ptr[i]); // Font is 5x7
        }
        
        if (is_selected_item && marqueeActive) {
          u8g2.drawStr(text_content_x_start + (int)marqueeOffset, text_content_y_baseline, marqueeText);
        } else {
          char* display_text_truncated = truncateText(current_tool_list_ptr[i], text_available_render_width, u8g2); // Font is 5x7
          int text_width_pixels = u8g2.getStrWidth(display_text_truncated);
          // Center truncated text within its available width
          u8g2.drawStr(text_content_x_start + (text_available_render_width - text_width_pixels)/2 , text_content_y_baseline, display_text_truncated);
        }
        // Restore master clip window for the next item's box drawing (if any) or culling
        u8g2.setClipWindow(0, grid_content_area_start_y_abs, 127, 63);
      }
      u8g2.setDrawColor(1); // Reset draw color for next item (if it's unselected)
    }
  }
  u8g2.setMaxClipWindow(); // Fully reset clipping at the end of the function
}

void drawCustomIcon(int x, int y, int iconType, bool isLarge) {
  switch (iconType) {
    case 0: // Tools Icon: Crossed Wrench and Screwdriver (Used for Main Menu "Tools")
      if (isLarge) { 
        u8g2.drawLine(x + 4, y + 11, x + 11, y + 4); 
        u8g2.drawBox(x + 2, y + 9, 4, 4);          
        u8g2.drawPixel(x+3, y+10); u8g2.drawPixel(x+4,y+9); 
        u8g2.drawBox(x + 10, y + 2, 4, 4);         
        u8g2.drawPixel(x+11,y+5); u8g2.drawPixel(x+12,y+4); 
        u8g2.drawLine(x + 4, y + 4, x + 11, y + 11); 
        u8g2.drawBox(x + 2, y + 2, 4, 4);          
        u8g2.drawBox(x + 10, y + 10, 4, 3);          
      } else { 
        u8g2.drawLine(x+1, y+5, x+4, y+2);
        u8g2.drawPixel(x+0,y+5); u8g2.drawPixel(x+1,y+6);
        u8g2.drawPixel(x+4,y+1); u8g2.drawPixel(x+5,y+2);
        u8g2.drawLine(x+1,y+1, x+5,y+5);
        u8g2.drawPixel(x+0,y+0); u8g2.drawPixel(x+1,y+0);u8g2.drawPixel(x+0,y+1); 
        u8g2.drawPixel(x+5,y+6); u8g2.drawPixel(x+6,y+5);u8g2.drawPixel(x+6,y+6); 
      }
      break;

    case 1: // Games Icon: Game Controller (Used for Main Menu "Games")
      if (isLarge) { 
        u8g2.drawRFrame(x + 1, y + 4, 14, 8, 3); 
        u8g2.drawBox(x + 3, y + 6, 2, 4); 
        u8g2.drawBox(x + 2, y + 7, 4, 2); 
        u8g2.drawDisc(x + 10, y + 7, 1);
        u8g2.drawDisc(x + 12, y + 9, 1);
      } else { 
        u8g2.drawRFrame(x + 0, y + 2, 8, 4, 1); 
        u8g2.drawPixel(x + 2, y + 3);          
        u8g2.drawPixel(x + 5, y + 4);          
      }
      break;

    case 2: // Settings Icon: Gear/Cog (Used for Main Menu "Settings")
      if (isLarge) { 
        u8g2.drawCircle(x + 8, y + 8, 5); 
        u8g2.drawCircle(x + 8, y + 8, 2); 
        for (int k = 0; k < 8; k++) { // Renamed loop variable to avoid conflict
          float angle = k * PI / 4.0;
          int tx1 = x + 8 + round(cos(angle) * 4); 
          int ty1 = y + 8 + round(sin(angle) * 4);
          u8g2.drawTriangle(tx1, ty1,
                             x + 8 + round(cos(angle + 0.15) * 7), y + 8 + round(sin(angle + 0.15) * 7),
                             x + 8 + round(cos(angle - 0.15) * 7), y + 8 + round(sin(angle - 0.15) * 7) );
        }
      } else { 
        u8g2.drawCircle(x + 4, y + 4, 3); 
        u8g2.drawPixel(x + 4, y + 4);     
        u8g2.drawPixel(x + 4, y + 0); u8g2.drawPixel(x + 4, y + 7);
        u8g2.drawPixel(x + 0, y + 4); u8g2.drawPixel(x + 7, y + 4);
        u8g2.drawPixel(x + 1, y + 1); u8g2.drawPixel(x + 6, y + 1);
        u8g2.drawPixel(x + 1, y + 6); u8g2.drawPixel(x + 6, y + 6);
      }
      break;

    case 3: // Info Icon: Letter 'i' inside a circle (Used for Main Menu "Info" and Settings "About Kiva")
      if (isLarge) { 
        u8g2.drawCircle(x + 8, y + 8, 7); 
        u8g2.drawDisc(x + 8, y + 4, 1.5);   
        u8g2.drawBox(x + 7, y + 7, 3, 6); 
      } else { 
        u8g2.drawCircle(x + 4, y + 4, 3);
        u8g2.drawPixel(x + 4, y + 2);    
        u8g2.drawVLine(x + 4, y + 3, 3); 
      }
      break;

    case 4: // Game Icon: Snake
      if (isLarge) { 
        u8g2.drawBox(x + 2, y + 7, 3, 3); u8g2.drawBox(x + 5, y + 7, 3, 3); 
        u8g2.drawBox(x + 8, y + 7, 3, 3); u8g2.drawBox(x + 11, y + 7, 3, 3); 
        u8g2.drawBox(x + 11, y + 4, 3, 3); 
        u8g2.drawBox(x + 11, y + 1, 3, 3);
        u8g2.drawPixel(x + 3, y+8); 
      } else { 
        u8g2.drawHLine(x + 1, y + 4, 4); 
        u8g2.drawVLine(x + 4, y + 2, 3); 
        u8g2.drawPixel(x + 1, y + 3); 
      }
      break;

    case 5: // Game Icon: Tetris blocks
      if (isLarge) { 
        u8g2.drawBox(x + 2, y + 9, 3, 3); u8g2.drawBox(x + 2, y + 6, 3, 3);
        u8g2.drawBox(x + 2, y + 3, 3, 3); u8g2.drawBox(x + 5, y + 9, 3, 3);
        u8g2.drawBox(x + 9, y + 5, 3, 3); u8g2.drawBox(x + 12, y + 5, 3, 3);
        u8g2.drawBox(x + 6, y + 5, 3, 3); u8g2.drawBox(x + 9, y + 2, 3, 3);
      } else { 
        u8g2.drawBox(x + 1, y + 4, 2, 2); u8g2.drawBox(x + 1, y + 2, 2, 2);
        u8g2.drawBox(x + 3, y + 4, 2, 2); 
        u8g2.drawBox(x + 5, y + 1, 2, 2); u8g2.drawBox(x + 5, y + 3, 2, 2); 
      }
      break;

    case 6: // Game Icon: Pong paddles and ball
      if (isLarge) { 
        u8g2.drawBox(x + 2, y + 4, 2, 8);  
        u8g2.drawBox(x + 12, y + 4, 2, 8); 
        u8g2.drawDisc(x + 8, y + 8, 1.5);    
      } else { 
        u8g2.drawVLine(x + 1, y + 2, 4); 
        u8g2.drawVLine(x + 6, y + 2, 4); 
        u8g2.drawPixel(x + 3, y + 3);    
      }
      break;

    case 7: // Game Icon: Maze
      if (isLarge) { 
        u8g2.drawFrame(x + 1, y + 1, 14, 14); 
        u8g2.drawVLine(x + 4, y + 1, 10);
        u8g2.drawVLine(x + 10, y + 4, 11);
        u8g2.drawHLine(x + 4, y + 4, 7);
        u8g2.drawHLine(x + 1, y + 10, 7);
        u8g2.drawPixel(x+13, y+2); 
      } else { 
        u8g2.drawFrame(x + 0, y + 0, 8, 8);
        u8g2.drawVLine(x + 2, y + 0, 5);
        u8g2.drawHLine(x + 2, y + 5, 4);
        u8g2.drawVLine(x + 5, y + 2, 6);
      }
      break;

    case 8: // Navigation Icon: Modernized Back Arrow (Chevron-like)
      if (isLarge) { 
        u8g2.drawLine(x + 10, y + 3, x + 4, y + 8); 
        u8g2.drawLine(x + 10, y + 4, x + 5, y + 8); 
        u8g2.drawLine(x + 10, y + 13, x + 4, y + 8); 
        u8g2.drawLine(x + 10, y + 12, x + 5, y + 8); 
      } else { 
        u8g2.drawLine(x + 5, y + 1, x + 2, y + 4);
        u8g2.drawLine(x + 5, y + 6, x + 2, y + 4);
      }
      break;

    case 9: // Network Icon: WiFi Symbol
      if (isLarge) { 
        int cx = x + 8; int cy = y + 12; 
        u8g2.drawDisc(cx, cy, 1.5); 
        u8g2.drawCircle(cx, cy, 4, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT); 
        u8g2.drawCircle(cx, cy, 7, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT); 
        u8g2.drawCircle(cx, cy, 10, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT); 
      } else { 
        int cx = x + 4; int cy = y + 6;
        u8g2.drawDisc(cx, cy, 0); 
        u8g2.drawCircle(cx, cy, 2, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT); 
        u8g2.drawCircle(cx, cy, 4, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT); 
      }
      break;

    case 10: // Network Icon: Bluetooth Symbol
      if (isLarge) { 
        u8g2.drawLine(x + 8, y + 1, x + 8, y + 15); 
        u8g2.drawLine(x + 8, y + 1, x + 13, y + 5); 
        u8g2.drawLine(x + 13, y + 5, x + 5, y + 10); 
        u8g2.drawLine(x + 5, y + 6, x + 13, y + 11); 
        u8g2.drawLine(x + 13, y + 11, x + 8, y + 15); 
      } else { 
        u8g2.drawLine(x + 4, y + 0, x + 4, y + 7);
        u8g2.drawLine(x + 4, y + 0, x + 6, y + 2);
        u8g2.drawLine(x + 6, y + 2, x + 2, y + 5);
        u8g2.drawLine(x + 2, y + 3, x + 6, y + 5); 
        u8g2.drawLine(x + 6, y + 5, x + 4, y + 7);
      }
      break;

    case 11: // Tool Icon: Jamming (Lightning Bolt)
      if (isLarge) { 
        u8g2.drawLine(x + 8, y + 2, x + 5, y + 7);
        u8g2.drawLine(x + 5, y + 7, x + 10, y + 6); 
        u8g2.drawLine(x + 10, y + 6, x + 6, y + 11);
        u8g2.drawLine(x + 6, y + 11, x + 11, y + 10); 
        u8g2.drawLine(x + 11, y + 10, x + 8, y + 14);
      } else { 
        u8g2.drawLine(x + 4, y + 0, x + 2, y + 3);
        u8g2.drawLine(x + 2, y + 3, x + 5, y + 3);
        u8g2.drawLine(x + 5, y + 3, x + 3, y + 6);
        u8g2.drawLine(x + 3, y + 6, x + 4, y + 7);
      }
      break;

    case 12: // Tool Icon: Injection (Syringe)
      if (isLarge) { 
        u8g2.drawLine(x + 3, y + 3, x + 8, y + 8); 
        u8g2.drawLine(x + 4, y + 3, x + 8, y + 7); 
        u8g2.drawRFrame(x + 7, y + 7, 6, 6, 1);    
        u8g2.drawBox(x + 9, y + 10, 2, 4);         
        u8g2.drawBox(x + 8, y + 13, 4, 2);        
      } else { 
        u8g2.drawLine(x + 1, y + 1, x + 3, y + 3);
        u8g2.drawFrame(x + 2, y + 2, 4, 4);
        u8g2.drawVLine(x + 4, y + 4, 3);
        u8g2.drawHLine(x + 3, y + 6, 3);
      }
      break;
    
    case 13: // Settings Icon: Display Options (Monitor/Screen)
      if (isLarge) { // 16x16
        u8g2.drawRFrame(x + 2, y + 2, 12, 9, 1); // Screen
        u8g2.drawBox(x + 5, y + 11, 6, 2);     // Stand base
        u8g2.drawVLine(x + 7, y + 11, 2);      // Stand neck part 1
        u8g2.drawVLine(x + 8, y + 11, 2);      // Stand neck part 2
      } else { // 8x8
        u8g2.drawFrame(x + 1, y + 1, 6, 4); // Screen
        u8g2.drawBox(x + 2, y + 5, 4, 1);   // Stand
        u8g2.drawVLine(x+3, y+5, 2);
      }
      break;

    case 14: // Settings Icon: Sound Setup (Speaker/Volume)
      if (isLarge) { // 16x16
        u8g2.drawBox(x + 3, y + 4, 5, 8); // Speaker body
        u8g2.drawTriangle(x + 7, y + 4, x + 7, y + 11, x + 10, y + 8); // Right part of speaker
        // Sound waves - Draw only the right half of the circles
        u8g2.drawCircle(x + 12, y + 8, 2, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_LOWER_RIGHT); // <--- CORRECTED
        u8g2.drawCircle(x + 13, y + 8, 4, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_LOWER_RIGHT); // <--- CORRECTED
      } else { // 8x8
        u8g2.drawBox(x + 1, y + 2, 3, 4); // Speaker body
        // Sound waves (simple lines)
        u8g2.drawLine(x + 5, y + 3, x + 6, y + 3);
        u8g2.drawLine(x + 5, y + 5, x + 7, y + 5);
      }
      break;

    case 15: // Settings Icon: System Info (Chip/CPU)
      if (isLarge) { // 16x16
        u8g2.drawRFrame(x + 3, y + 3, 10, 10, 1); // Chip body
        u8g2.drawRFrame(x + 5, y + 5, 6, 6, 0);   // Inner square
        // Pins (simplified)
        for(int k=0; k<3; ++k) {
            u8g2.drawHLine(x+0, y+4+(k*3), 3); // Left pins
            u8g2.drawHLine(x+13, y+4+(k*3), 3); // Right pins
            u8g2.drawVLine(x+4+(k*3), y+0, 3); // Top pins
            u8g2.drawVLine(x+4+(k*3), y+13, 3); // Bottom pins
        }
      } else { // 8x8
        u8g2.drawFrame(x + 1, y + 1, 6, 6); // Chip body
        u8g2.drawFrame(x + 2, y + 2, 4, 4); // Inner
        u8g2.drawPixel(x+0,y+3); u8g2.drawPixel(x+7,y+3); // Side pins
        u8g2.drawPixel(x+3,y+0); u8g2.drawPixel(x+3,y+7); // Top/bottom pins
      }
      break;
    

    case 16: // UI Icon: Refresh / Rescan (circular arrow)
      if (isLarge) { // 16x16 version - for main menu or larger contexts if needed
        u8g2.drawCircle(x + 8, y + 8, 6, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_LOWER_RIGHT | U8G2_DRAW_LOWER_LEFT); // 3/4 circle
        u8g2.drawTriangle(x + 8, y + 2 -1, x + 8 - 3, y + 2 + 2, x + 8 + 3, y + 2 + 2); // Arrowhead at top pointing up-ish
        // Correct arrowhead for top-left position opening:
        u8g2.drawLine(x+8, y+2, x+5, y+3); // line from center of opening to point
        u8g2.drawLine(x+5,y+3, x+6, y+5);  // left barb
        u8g2.drawLine(x+5,y+3, x+3, y+5);  // right barb
      } else { // 8x8 version - for lists
        // Draw a smaller 3/4 circle
        u8g2.drawCircle(x + 4, y + 4, 3, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_LOWER_RIGHT | U8G2_DRAW_LOWER_LEFT);
        // Arrowhead at the top-left opening of the circle, pointing left-ish
        u8g2.drawLine(x + 4, y + 1, x + 4 - 2, y + 1 + 1); // Upper part of arrowhead
        u8g2.drawLine(x + 4, y + 1, x + 4 - 1, y + 1 + 2); // Lower part of arrowhead
      }
      break;
    
        case 17: // UI Icon: Small Lightning Bolt (for charging status)
      // This icon is intended to be small, around 5-7px wide/high.
      // 'isLarge' flag might be ignored or used for a slightly bolder version if needed.
      // For status bar, 'isLarge = false' is expected.
      if (isLarge) { // Larger version (e.g. 10x10 or 12x12 for other UI elements if needed)
        // Draw a more prominent lightning bolt
        u8g2.drawLine(x + 5, y + 0, x + 2, y + 5);
        u8g2.drawLine(x + 2, y + 5, x + 6, y + 4);
        u8g2.drawLine(x + 6, y + 4, x + 3, y + 9);
        u8g2.drawLine(x + 3, y + 9, x + 7, y + 8);
        u8g2.drawLine(x + 7, y + 8, x + 4, y + 13); // Example larger size
      } else { // Small version for status bar (target ~6px width, ~8px height)
        // Top point
        // u8g2.drawPixel(x + 2, y + 0);
        // Upper segment
        u8g2.drawLine(x + 2, y + 0, x + 0, y + 3); // Slant down-left
        // Middle segment
        u8g2.drawLine(x + 0, y + 3, x + 3, y + 2); // Slant up-right slightly
        // Lower segment
        u8g2.drawLine(x + 3, y + 2, x + 1, y + 5); // Slant down-left
        // Bottom point (optional, can make it look too long)
        // u8g2.drawPixel(x + 3, y + 7);
        // A more compact version for 8px height:
        u8g2.drawLine(x+3, y+0, x+1, y+3); // Top zig
        u8g2.drawLine(x+1, y+3, x+4, y+3); // Middle zag
        u8g2.drawLine(x+4, y+3, x+2, y+6); // Bottom zig
      }

    default:
      if (isLarge) {
        u8g2.drawFrame(x + 2, y + 2, 12, 12);
      } else {
        u8g2.drawFrame(x + 2, y + 2, 4, 4);
      }
      break;
  }
}

void drawRndBox(int x, int y, int w, int h, int r, bool fill) {
    if (w <= 0 || h <= 0) return; // Don't draw if width or height is zero or negative
    if (w <= 2 * r || h <= 2 * r) { // If too small for radius, draw as simple box/frame
        if (fill) u8g2.drawBox(x, y, w, h);
        else u8g2.drawFrame(x, y, w, h);
        return;
    }
    if (fill) u8g2.drawRBox(x, y, w, h, r);
    else u8g2.drawRFrame(x, y, w, h, r);
}

void drawBatIcon(int x_left, int y_top, uint8_t percentage) {
    // Draws a simplified, blocky battery icon.
    // x_left: Leftmost X coordinate of the icon.
    // y_top: Topmost Y coordinate of the icon.
    // Icon total width: 9px, height: 5px (body) + 1px (tip height) = 5px overall effective height for alignment.

    int body_width = 7;  // Main body
    int body_height = 5;
    int tip_width = 1;   // Tip of the battery (single pixel wide)
    int tip_height = 3;  // Tip height (centered on body height)

    // Main battery body (simple frame)
    u8g2.drawFrame(x_left, y_top, body_width, body_height);

    // Battery tip (small rectangle on the right side, vertically centered)
    int tip_y = y_top + (body_height - tip_height) / 2;
    u8g2.drawBox(x_left + body_width, tip_y, tip_width, tip_height); // Use drawBox for a solid tip

    // Inner fill based on percentage
    // Fill area is inside the body, with 1px padding
    int fill_padding_x = 1;
    int fill_padding_y = 1;
    int max_fill_width = body_width - 2 * fill_padding_x;
    int fill_width = (percentage * max_fill_width) / 100;

    if (fill_width > 0) {
        u8g2.drawBox(x_left + fill_padding_x,            // X start of fill
                       y_top + fill_padding_y,             // Y start of fill
                       fill_width,                         // Width of fill
                       body_height - 2 * fill_padding_y);  // Height of fill
    }
}