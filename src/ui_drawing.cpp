#include "ui_drawing.h"
#include "battery_monitor.h"
#include "menu_logic.h"
#include "pcf_utils.h"
#include "keyboard_layout.h"
#include "wifi_manager.h"
#include "jamming.h"
#include "ota_manager.h"  // For OTA status variables
#include "firmware_metadata.h"
#include <Arduino.h>
#include <math.h>

// Externs for Wi-Fi data (defined in KivaMain.ino)
extern WifiNetwork scannedNetworks[MAX_WIFI_NETWORKS];
extern int foundWifiNetworksCount;
extern int wifiMenuIndex;


// Animation struct methods
void VerticalListAnimation::init() {
  for (int i = 0; i < MAX_ANIM_ITEMS; i++) {
    itemOffsetY[i] = 0;
    itemScale[i] = 0.0f;
    targetOffsetY[i] = 0;
    targetScale[i] = 0.0f;
    introStartSourceOffsetY[i] = 0.0f;
    introStartSourceScale[i] = 0.0f;
  }
  isIntroPhase = false;
  introStartTime = 0;
}

void VerticalListAnimation::setTargets(int selIdx, int total) {
  for (int i = 0; i < MAX_ANIM_ITEMS; i++) {
    if (i < total) {
      int rP = i - selIdx;
      targetOffsetY[i] = rP * itmSpc;  // itmSpc will be the adjusted value
      if (i == selIdx) targetScale[i] = 1.3f;
      else if (abs(rP) == 1) targetScale[i] = 1.f;
      else targetScale[i] = 0.8f;
    } else {
      targetOffsetY[i] = (i - selIdx) * itmSpc;
      targetScale[i] = 0.f;
    }
  }
}

void VerticalListAnimation::startIntro(int selIdx, int total) {  // Removed commonInitialYOffset, commonInitialScale params
  isIntroPhase = true;
  introStartTime = millis();
  const float initial_scale = 0.0f;
  // How much each item is initially offset from its final Y, towards the center of the animation.
  // A positive value means items above final target start lower, items below final target start higher.
  const float initial_y_offset_factor_from_final = 0.25f;  // e.g. 25% of their final offset from center, but towards center

  for (int i = 0; i < MAX_ANIM_ITEMS; i++) {
    if (i < total) {
      int rP = i - selIdx;  // relativePosition to selected item

      // Calculate final target state for this item
      float finalTargetOffsetY = rP * itmSpc;
      float finalTargetScale;
      if (i == selIdx) finalTargetScale = 1.3f;
      else if (abs(rP) == 1) finalTargetScale = 1.f;
      else finalTargetScale = 0.8f;

      // Set intro start sources (where the animation begins for this item)
      // Initial Y: Start closer to the center (y=0 in relative terms) and then expand outwards.
      // If finalTargetOffsetY is -20, and factor is 0.25, it means it starts 0.25*(-20) = -5 from *itself* towards center.
      // So, if finalTargetOffsetY is negative (item is above center), initial_y_offset_factor_from_final * finalTargetOffsetY is positive.
      // Example: finalY = -18. It starts at -18 + (0.25 * 18) = -18 + 4.5 = -13.5 (closer to 0)
      // Example: finalY = 18. It starts at 18 - (0.25 * 18) = 18 - 4.5 = 13.5 (closer to 0)
      introStartSourceOffsetY[i] = finalTargetOffsetY * (1.0f - initial_y_offset_factor_from_final);

      // For the selected item (rP=0), its finalTargetOffsetY is 0, so introStartSourceOffsetY remains 0. It scales in place.
      // This creates an effect of items expanding from the selected item's line.

      introStartSourceScale[i] = initial_scale;

      // Set current animation values to these starting points
      itemOffsetY[i] = introStartSourceOffsetY[i];
      itemScale[i] = introStartSourceScale[i];

      // Store the calculated final targets (where items will animate TO)
      targetOffsetY[i] = finalTargetOffsetY;
      targetScale[i] = finalTargetScale;

    } else {                                               // Items not part of the active list should remain invisible
      introStartSourceOffsetY[i] = (i - selIdx) * itmSpc;  // Keep some consistent off-screen Y
      introStartSourceScale[i] = 0.0f;
      itemOffsetY[i] = introStartSourceOffsetY[i];
      itemScale[i] = introStartSourceScale[i];
      targetOffsetY[i] = (i - selIdx) * itmSpc;
      targetScale[i] = 0.0f;
    }
  }
}

bool VerticalListAnimation::update() {
  bool animActive = false;
  unsigned long currentTime = millis();

  if (isIntroPhase) {
    float progress = 0.0f;
    if (introStartTime > 0 && currentTime > introStartTime && introDuration > 0) {
      progress = (float)(currentTime - introStartTime) / introDuration;
    }

    if (progress < 0.0f) progress = 0.0f;

    float easedProgress = 1.0f - pow(1.0f - progress, 3);  // Ease-out cubic

    if (progress >= 1.0f) {
      easedProgress = 1.0f;
    }

    for (int i = 0; i < MAX_ANIM_ITEMS; i++) {
      itemOffsetY[i] = introStartSourceOffsetY[i] + (targetOffsetY[i] - introStartSourceOffsetY[i]) * easedProgress;
      itemScale[i] = introStartSourceScale[i] + (targetScale[i] - introStartSourceScale[i]) * easedProgress;
    }

    if (progress >= 1.0f) {
      isIntroPhase = false;
      for (int i = 0; i < MAX_ANIM_ITEMS; i++) {  // Snap to final target values
        itemOffsetY[i] = targetOffsetY[i];
        itemScale[i] = targetScale[i];
      }
      animActive = false;
    } else {
      animActive = true;
    }
  } else {
    for (int i = 0; i < MAX_ANIM_ITEMS; i++) {
      float oD = targetOffsetY[i] - itemOffsetY[i];
      if (abs(oD) > 0.01f) {
        itemOffsetY[i] += oD * animSpd * frmTime;
        animActive = true;
      } else {
        itemOffsetY[i] = targetOffsetY[i];
      }

      float sD = targetScale[i] - itemScale[i];
      if (abs(sD) > 0.001f) {
        itemScale[i] += sD * animSpd * frmTime;
        animActive = true;
      } else {
        itemScale[i] = targetScale[i];
      }
    }
  }
  return animActive;
}

void CarouselAnimation::init() {
  for (int i = 0; i < MAX_ANIM_ITEMS; i++) {
    itemOffsetX[i] = 0;
    itemScale[i] = 0.f;
    targetOffsetX[i] = 0;
    targetScale[i] = 0.f;
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
    if (abs(oD) > 0.1f) {
      itemOffsetX[i] += oD * animSpd * frmTime;
      anim = true;
    } else {
      itemOffsetX[i] = targetOffsetX[i];
    }
    float sD = targetScale[i] - itemScale[i];
    if (abs(sD) > 0.01f) {
      itemScale[i] += sD * animSpd * frmTime;
      anim = true;
    } else {
      itemScale[i] = targetScale[i];
    }
  }
  return anim;
}


void drawWifiSignalStrength(int x, int y_icon_top, int8_t rssi) {
  int num_bars_to_draw = 0;
  if (rssi >= -55) num_bars_to_draw = 4;
  else if (rssi >= -80) num_bars_to_draw = 3;
  else if (rssi >= -90) num_bars_to_draw = 2;
  else if (rssi >= -100) num_bars_to_draw = 1;

  int bar_width = 2;
  int bar_spacing = 1;
  int first_bar_height = 2;
  int bar_height_increment = 2;
  int max_icon_height = first_bar_height + (4 - 1) * bar_height_increment;

  for (int i = 0; i < num_bars_to_draw; i++) {
    int current_bar_height = first_bar_height + i * bar_height_increment;
    int bar_x_position = x + i * (bar_width + bar_spacing);
    int y_pos_for_drawing_this_bar = y_icon_top + (max_icon_height - current_bar_height);
    u8g2.drawBox(bar_x_position, y_pos_for_drawing_this_bar, bar_width, current_bar_height);
  }
}


void drawWifiSetupScreen() {
  // ... (Implementation unchanged) ...
  const int list_start_y_abs = STATUS_BAR_H + 1;
  const int list_visible_height = u8g2.getDisplayHeight() - list_start_y_abs;
  const int item_row_h = WIFI_LIST_ITEM_H;
  const int item_padding_x = 2;
  const int content_padding_x = 4;

  u8g2.setClipWindow(0, list_start_y_abs, u8g2.getDisplayWidth(), u8g2.getDisplayHeight());

  float target_scroll_for_centering = 0;
  if (maxMenuItems > 0 && (maxMenuItems * item_row_h > list_visible_height)) {
    target_scroll_for_centering = (wifiMenuIndex * item_row_h) - (list_visible_height / 2) + (item_row_h / 2);
    if (target_scroll_for_centering < 0) target_scroll_for_centering = 0;
    float max_scroll_val = (maxMenuItems * item_row_h) - list_visible_height;
    if (max_scroll_val < 0) max_scroll_val = 0;
    if (target_scroll_for_centering > max_scroll_val) {
      target_scroll_for_centering = max_scroll_val;
    }
  }
  float scrollDiff = target_scroll_for_centering - currentWifiListScrollOffset_Y_anim;
  if (abs(scrollDiff) > 0.1f) {
    currentWifiListScrollOffset_Y_anim += scrollDiff * GRID_ANIM_SPEED * 0.016f;
  } else {
    currentWifiListScrollOffset_Y_anim = target_scroll_for_centering;
  }


  if (!wifiHardwareEnabled) {
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setDrawColor(1);
    const char* offMsg = getWifiStatusMessage();
    int msgWidth = u8g2.getStrWidth(offMsg);
    int text_y_pos = list_start_y_abs + (list_visible_height - (u8g2.getAscent() - u8g2.getDescent())) / 2 + u8g2.getAscent();
    u8g2.drawStr((u8g2.getDisplayWidth() - msgWidth) / 2, text_y_pos, offMsg);
  } else if (wifiIsScanning) {
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setDrawColor(1);
    const char* scanMsg = getWifiStatusMessage();
    int msgWidth = u8g2.getStrWidth(scanMsg);
    int text_y_pos = list_start_y_abs + (list_visible_height - (u8g2.getAscent() - u8g2.getDescent())) / 2 + u8g2.getAscent();
    u8g2.drawStr((u8g2.getDisplayWidth() - msgWidth) / 2, text_y_pos, scanMsg);
  } else {
    wifiListAnim.update();
    u8g2.setFont(u8g2_font_6x10_tf);

    if (foundWifiNetworksCount == 0 && maxMenuItems <= 2) {
      u8g2.setDrawColor(1);
      const char* noNetMsg = getWifiStatusMessage();
      int msgWidth = u8g2.getStrWidth(noNetMsg);
      int text_y_pos = list_start_y_abs + (list_visible_height - (u8g2.getAscent() - u8g2.getDescent())) / 2 + u8g2.getAscent();
      u8g2.drawStr((u8g2.getDisplayWidth() - msgWidth) / 2, text_y_pos, noNetMsg);
    }


    for (int i = 0; i < maxMenuItems; i++) {
      int item_center_y_in_full_list = (i * item_row_h) + (item_row_h / 2);
      int item_center_on_screen_y = list_start_y_abs + item_center_y_in_full_list - (int)currentWifiListScrollOffset_Y_anim;
      int item_top_on_screen_y = item_center_on_screen_y - item_row_h / 2;

      if (item_top_on_screen_y + item_row_h < list_start_y_abs || item_top_on_screen_y >= u8g2.getDisplayHeight()) {
        continue;
      }

      bool is_selected_item = (i == wifiMenuIndex);

      if (is_selected_item) {
        u8g2.setDrawColor(1);
        drawRndBox(item_padding_x, item_top_on_screen_y, u8g2.getDisplayWidth() - 2 * item_padding_x, item_row_h, 2, true);
        u8g2.setDrawColor(0);
      } else {
        u8g2.setDrawColor(1);
      }

      const char* currentItemTextPtr = nullptr;
      bool isNetworkItem = false;
      int8_t currentRssi = 0;
      bool currentIsSecure = false;
      int action_icon_type = -1;
      String displayText;

      if (i < foundWifiNetworksCount) {
        if (currentConnectedSsid.length() > 0 && currentConnectedSsid.equals(scannedNetworks[i].ssid)) {
          displayText = "* ";
          displayText += scannedNetworks[i].ssid;
        } else {
          displayText = scannedNetworks[i].ssid;
        }
        currentItemTextPtr = displayText.c_str();

        currentRssi = scannedNetworks[i].rssi;
        currentIsSecure = scannedNetworks[i].isSecure;
        isNetworkItem = true;
      } else if (i == foundWifiNetworksCount) {
        currentItemTextPtr = "Scan Again";
        action_icon_type = 16;
      } else if (i == foundWifiNetworksCount + 1) {
        currentItemTextPtr = "Back";
        action_icon_type = 8;
      }

      if (currentItemTextPtr == nullptr || currentItemTextPtr[0] == '\0') continue;

      int current_content_x = content_padding_x + item_padding_x;
      int icon_y_center_for_draw = item_center_on_screen_y;

      if (isNetworkItem) {
        drawWifiSignalStrength(current_content_x, icon_y_center_for_draw - 4, currentRssi);
        current_content_x += 12;

        if (currentIsSecure) {
          int lock_body_start_x = current_content_x;
          int body_top_y = icon_y_center_for_draw - 3;
          u8g2.drawBox(lock_body_start_x, body_top_y, 4, 4);
          u8g2.drawHLine(lock_body_start_x + 1, body_top_y - 2, 2);
          u8g2.drawPixel(lock_body_start_x, body_top_y - 1);
          u8g2.drawPixel(lock_body_start_x + 3, body_top_y - 1);
          current_content_x += 6;
        } else {
          current_content_x += 6;
        }
      } else if (action_icon_type != -1) {
        drawCustomIcon(current_content_x, icon_y_center_for_draw - 4, action_icon_type, false);
        current_content_x += 10;
      } else {
        current_content_x += 10;
      }

      int text_x_start = current_content_x;
      int text_available_width = u8g2.getDisplayWidth() - text_x_start - content_padding_x - item_padding_x;
      int text_baseline_y = item_center_on_screen_y - (u8g2.getAscent() + u8g2.getDescent()) / 2 + u8g2.getAscent();

      if (is_selected_item && isNetworkItem) {
        updateMarquee(text_available_width, currentItemTextPtr);
        if (marqueeActive) {
          u8g2.drawStr(text_x_start + (int)marqueeOffset, text_baseline_y, marqueeText);
        } else {
          char* truncated = truncateText(currentItemTextPtr, text_available_width, u8g2);
          u8g2.drawStr(text_x_start, text_baseline_y, truncated);
        }
      } else {
        char* truncated = truncateText(currentItemTextPtr, text_available_width, u8g2);
        u8g2.drawStr(text_x_start, text_baseline_y, truncated);
      }
    }
  }
  u8g2.setDrawColor(1);
  u8g2.setMaxClipWindow();
}

void drawPasswordInputScreen() {
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setDrawColor(1);

  char ssidTitle[40];
  snprintf(ssidTitle, sizeof(ssidTitle), "Enter Pass for:");
  u8g2.drawStr(2, 7 + STATUS_BAR_H, ssidTitle); // Y position for "Enter Pass for:"

  char truncatedSsid[22]; // Assuming max 21 chars + null for SSID display here
  strncpy(truncatedSsid, currentSsidToConnect, sizeof(truncatedSsid) - 1);
  truncatedSsid[sizeof(truncatedSsid) - 1] = '\0';
  // Truncate if too long for display
  if (u8g2.getStrWidth(truncatedSsid) > u8g2.getDisplayWidth() - 4) {
    // truncateText uses SBUF, so copy result if needed elsewhere, or use directly if SBUF is safe
    char* tempTrunc = truncateText(currentSsidToConnect, u8g2.getDisplayWidth() - 4, u8g2);
    strncpy(truncatedSsid, tempTrunc, sizeof(truncatedSsid) - 1);
    truncatedSsid[sizeof(truncatedSsid) - 1] = '\0';
  }
  int ssidTextY = 19 + STATUS_BAR_H; // Y position for the SSID itself
  u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(truncatedSsid)) / 2, ssidTextY, truncatedSsid);

  // Define field properties
  int fieldX = 5;
  // int fieldY = PASSWORD_INPUT_FIELD_Y; // Original
  int fieldY = PASSWORD_INPUT_FIELD_Y + 5; // MODIFIED: Moved down by 5 pixels
  int fieldW = u8g2.getDisplayWidth() - 10;
  int fieldH = PASSWORD_INPUT_FIELD_H;

  // Ensure fieldY doesn't push it too far down, adjust if necessary
  // For example, if it collides with the instruction text at the bottom:
  int instructionTextY_approx = u8g2.getDisplayHeight() - 2 - u8g2.getAscent(); // Approximate top of instruction text
  if (fieldY + fieldH > instructionTextY_approx - 2) { // If field bottom is too close to instruction top
      fieldY = instructionTextY_approx - fieldH - 2; // Adjust upwards slightly
      if (fieldY < ssidTextY + u8g2.getAscent() + 2) { // Ensure it's still below SSID text
          fieldY = ssidTextY + u8g2.getAscent() + 2;
      }
  }


  u8g2.drawFrame(fieldX, fieldY, fieldW, fieldH);

  char maskedPassword[PASSWORD_MAX_LEN + 1];
  int len = strlen(wifiPasswordInput);
  for (int i = 0; i < len; ++i) maskedPassword[i] = '*';
  maskedPassword[len] = '\0';

  int textX = fieldX + 3;
  // Adjust textY to be vertically centered within the new fieldY
  int textY = fieldY + (fieldH - (u8g2.getAscent() - u8g2.getDescent())) / 2 + u8g2.getAscent();

  int maxPassDisplayLengthChars = (fieldW - 6) / u8g2.getStrWidth("*"); 
  if (maxPassDisplayLengthChars < 1) maxPassDisplayLengthChars = 1;

  int passDisplayStartIdx = 0;
  if (wifiPasswordInputCursor >= maxPassDisplayLengthChars) {
    passDisplayStartIdx = wifiPasswordInputCursor - maxPassDisplayLengthChars + 1;
  }

  char displaySegment[maxPassDisplayLengthChars + 2]; // +1 for potential char, +1 for null
  strncpy(displaySegment, maskedPassword + passDisplayStartIdx, maxPassDisplayLengthChars);
  displaySegment[maxPassDisplayLengthChars] = '\0'; // Ensure null termination
  u8g2.drawStr(textX, textY, displaySegment);

  // Calculate cursor X position relative to the start of the displayed segment
  int cursorDisplayPosInSegment = wifiPasswordInputCursor - passDisplayStartIdx;
  int cursorX = textX; 
  if (cursorDisplayPosInSegment > 0) {
    // Create a substring of the *displayed* segment up to the cursor
    char tempCursorSubstr[maxPassDisplayLengthChars + 1];
    strncpy(tempCursorSubstr, displaySegment, cursorDisplayPosInSegment);
    tempCursorSubstr[cursorDisplayPosInSegment] = '\0';
    cursorX += u8g2.getStrWidth(tempCursorSubstr);
  }

  // Draw blinking cursor
  if ((millis() / 500) % 2 == 0) {
    u8g2.drawVLine(cursorX, fieldY + 2, fieldH - 4);
  }

  // Instruction text at the bottom (unchanged logic, but its position might influence fieldY adjustment)
  u8g2.setFont(u8g2_font_5x7_tf);
  const char* instrOriginal = "See aux display for keyboard";
  int instrY = u8g2.getDisplayHeight() - 2; 
  int availableInstrWidth = u8g2.getDisplayWidth() - 4;

  updateMarquee(availableInstrWidth, instrOriginal);

  if (marqueeActive && strcmp(marqueeText, instrOriginal) == 0) {
    int text_render_x = (u8g2.getDisplayWidth() - u8g2.getStrWidth(marqueeText)) / 2;
    if (marqueeTextLenPx > availableInstrWidth) {
      text_render_x = 2 + (int)marqueeOffset;
    }
    u8g2.drawStr(text_render_x, instrY, marqueeText);
  } else {
    char* truncatedInstr = truncateText(instrOriginal, availableInstrWidth, u8g2);
    u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(truncatedInstr)) / 2, instrY, truncatedInstr);
  }
}

void drawKeyboardOnSmallDisplay() {
  // ... (Implementation unchanged) ...
  u8g2_small.clearBuffer();
  u8g2_small.setFont(u8g2_font_5x7_tf);

  const KeyboardKey* currentLayout = getCurrentKeyboardLayout(currentKeyboardLayer);
  if (!currentLayout) return;

  const int keyHeight = 8;
  const int keyWidthBase = 12;
  const int interKeySpacingX = 1;
  const int interKeySpacingY = 0;

  int currentX, currentY = 0;

  for (int r = 0; r < KB_ROWS; ++r) {
    currentX = 0;
    currentY = r * keyHeight;
    for (int c = 0; c < KB_LOGICAL_COLS;) {
      const KeyboardKey& key = getKeyFromLayout(currentLayout, r, c);
      if (key.value == KB_KEY_NONE && key.displayChar == 0) {
        c++;
        continue;
      }
      if (key.colSpan == 0) {
        c++;
        continue;
      }

      int actualKeyWidth = key.colSpan * keyWidthBase + (key.colSpan - 1) * interKeySpacingX;
      if (currentX + actualKeyWidth > u8g2_small.getDisplayWidth()) {
        actualKeyWidth = u8g2_small.getDisplayWidth() - currentX;
      }
      if (actualKeyWidth <= 0) {
        c += key.colSpan;
        continue;
      }

      bool isFocused = (r == keyboardFocusRow && c == keyboardFocusCol);

      if (isFocused) {
        u8g2_small.setDrawColor(1);
        u8g2_small.drawBox(currentX, currentY, actualKeyWidth, keyHeight);
        u8g2_small.setDrawColor(0);
      } else {
        u8g2_small.setDrawColor(1);
        u8g2_small.drawFrame(currentX, currentY, actualKeyWidth, keyHeight);
      }

      char displayCharStr[2] = { 0 };
      if (key.displayChar != 0) {
        displayCharStr[0] = key.displayChar;
      } else {
        switch (key.value) {
          case KB_KEY_BACKSPACE: displayCharStr[0] = '<'; break;
          case KB_KEY_ENTER: displayCharStr[0] = 'E'; break;
          case KB_KEY_SHIFT: displayCharStr[0] = (capsLockActive || (currentKeyboardLayer == KB_LAYER_UPPERCASE && !capsLockActive)) ? 's' : 'S'; break;
          case KB_KEY_SPACE: break;
          case KB_KEY_TO_NUM: displayCharStr[0] = '1'; break;
          case KB_KEY_TO_SYM: displayCharStr[0] = '%'; break;
          case KB_KEY_TO_LOWER:
          case KB_KEY_TO_UPPER: displayCharStr[0] = 'a'; break;
          default:
            if (key.value > 0 && key.value < 127) displayCharStr[0] = (char)key.value;
            break;
        }
      }

      if (displayCharStr[0] != 0) {
        int charWidth = u8g2_small.getStrWidth(displayCharStr);
        int textX = currentX + (actualKeyWidth - charWidth) / 2;
        int textY = currentY + 6;
        u8g2_small.drawStr(textX, textY, displayCharStr);
      }
      currentX += actualKeyWidth + interKeySpacingX;
      c += key.colSpan;
    }
  }
  u8g2_small.setDrawColor(1);
  u8g2_small.sendBuffer();
}


void drawWifiConnectingStatusScreen() {
  // ... (Implementation unchanged) ...
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setDrawColor(1);
  const char* msg1 = "Connecting to:";
  u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(msg1)) / 2, 20, msg1);

  char truncatedSsid[22];
  strncpy(truncatedSsid, currentSsidToConnect, sizeof(truncatedSsid) - 1);
  truncatedSsid[sizeof(truncatedSsid) - 1] = '\0';
  if (u8g2.getStrWidth(truncatedSsid) > u8g2.getDisplayWidth() - 20) {
    truncateText(currentSsidToConnect, u8g2.getDisplayWidth() - 20, u8g2);
    strncpy(truncatedSsid, SBUF, sizeof(truncatedSsid) - 1);
    truncatedSsid[sizeof(truncatedSsid) - 1] = '\0';
  }
  u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(truncatedSsid)) / 2, 32, truncatedSsid);

  int dots = (millis() / 300) % 4;
  char ellipsis[4] = "";
  for (int i = 0; i < dots; ++i) ellipsis[i] = '.';
  ellipsis[dots] = '\0';

  char connectingMsg[20];
  snprintf(connectingMsg, sizeof(connectingMsg), "Please wait%s", ellipsis);
  u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(connectingMsg)) / 2, 50, connectingMsg);
}

void drawWifiConnectionResultScreen() {
  // ... (Implementation unchanged) ...
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setDrawColor(1);

  const char* resultMsg = getWifiStatusMessage();

  int msgWidth = u8g2.getStrWidth(resultMsg);
  u8g2.drawStr((u8g2.getDisplayWidth() - msgWidth) / 2, 32, resultMsg);

  WifiConnectionStatus lastStatus = getCurrentWifiConnectionStatus();
  if (lastStatus == WIFI_CONNECTED_SUCCESS) {
    drawCustomIcon(u8g2.getDisplayWidth() / 2 - 8, 10, 3, true);
  } else {
    drawCustomIcon(u8g2.getDisplayWidth() / 2 - 8, 10, 11, true);
  }
}

void drawWifiDisconnectOverlay() {
  // --- Animation Update ---
  if (disconnectOverlayAnimatingIn) {
    float diff = disconnectOverlayTargetScale - disconnectOverlayCurrentScale;
    if (abs(diff) > 0.01f) {
      // Using a slightly faster animation speed for the overlay pop-up
      disconnectOverlayCurrentScale += diff * (GRID_ANIM_SPEED * 1.5f) * 0.016f;
      if ((diff > 0 && disconnectOverlayCurrentScale > disconnectOverlayTargetScale) || (diff < 0 && disconnectOverlayCurrentScale < disconnectOverlayTargetScale)) {
        disconnectOverlayCurrentScale = disconnectOverlayTargetScale;
      }
    } else {
      disconnectOverlayCurrentScale = disconnectOverlayTargetScale;
      disconnectOverlayAnimatingIn = false;  // Animation finished
    }
  }

  if (disconnectOverlayCurrentScale <= 0.05f && !disconnectOverlayAnimatingIn) return;  // Don't draw if effectively invisible and not animating

  // --- Base (Final) Dimensions & Position ---
  int baseOverlayWidth = 100;
  int baseOverlayHeight = 50;  // Slightly increased height for better spacing
  int baseOverlayX = (u8g2.getDisplayWidth() - baseOverlayWidth) / 2;
  int baseOverlayY = (u8g2.getDisplayHeight() - baseOverlayHeight) / 2;

  // Ensure it's below the status bar
  int minOverlayY = STATUS_BAR_H + 3;  // 3px margin below status bar
  baseOverlayY = max(baseOverlayY, minOverlayY);

  // --- Animated Dimensions & Positions (scaling from center) ---
  int animatedOverlayWidth = (int)(baseOverlayWidth * disconnectOverlayCurrentScale);
  int animatedOverlayHeight = (int)(baseOverlayHeight * disconnectOverlayCurrentScale);
  if (animatedOverlayWidth < 1 || animatedOverlayHeight < 1) return;  // Don't draw if too small

  int drawX = baseOverlayX + (baseOverlayWidth - animatedOverlayWidth) / 2;
  int drawY = baseOverlayY + (baseOverlayHeight - animatedOverlayHeight) / 2;

  // --- Drawing ---
  int padding = 4;
  u8g2.setFont(u8g2_font_6x10_tf);
  int fontAscent = u8g2.getAscent();
  int textLineHeight = fontAscent + 3;  // Ascent + small gap

  // 1. Draw the black border (slightly larger than the white box if desired, or same size)
  // For a simple pop-out, we'll draw the white box, then a black frame around it.
  // To make it look like a black border *around* a white box:
  // Draw black frame, then slightly smaller white filled box.
  // Or, simpler: draw white filled box, then black frame of same size.

  // Draw filled white background
  u8g2.setDrawColor(1);  // White
  u8g2.drawRBox(drawX, drawY, animatedOverlayWidth, animatedOverlayHeight, 3);

  // Draw black border around the white box
  u8g2.setDrawColor(0);  // Black
  u8g2.drawRFrame(drawX, drawY, animatedOverlayWidth, animatedOverlayHeight, 3);


  // Only draw content if the overlay is mostly scaled up
  if (disconnectOverlayCurrentScale > 0.9f) {
    u8g2.setDrawColor(0);  // Black text on white background

    // Title
    const char* title = "Disconnect";
    int titleX = drawX + (animatedOverlayWidth - u8g2.getStrWidth(title)) / 2;
    int titleY = drawY + padding + fontAscent;
    u8g2.drawStr(titleX, titleY, title);

    // SSID (truncated)
    char truncatedSsid[18];
    truncateText(currentSsidToConnect, animatedOverlayWidth - 2 * padding - 4, u8g2);
    strncpy(truncatedSsid, SBUF, sizeof(truncatedSsid) - 1);
    truncatedSsid[sizeof(truncatedSsid) - 1] = '\0';
    int ssidX = drawX + (animatedOverlayWidth - u8g2.getStrWidth(truncatedSsid)) / 2;
    int ssidY = titleY + textLineHeight;
    u8g2.drawStr(ssidX, ssidY, truncatedSsid);

    // Options (X and O)
    int optionTextY = drawY + animatedOverlayHeight - padding - fontAscent - 1;
    int optionBoxY = optionTextY - fontAscent - 2;  // Top of option boxes

    const char* cancelText = "X";
    const char* confirmText = "O";
    int cancelWidth = u8g2.getStrWidth(cancelText);
    int confirmWidth = u8g2.getStrWidth(confirmText);
    int optionBoxWidth = max(cancelWidth, confirmWidth) + 12;
    int optionBoxHeight = fontAscent + 4;  // Box height based on font ascent + padding

    // Calculate X positions for centered options
    int totalOptionsWidth = 2 * optionBoxWidth + 10;  // 10px spacing between options
    int optionsStartX = drawX + (animatedOverlayWidth - totalOptionsWidth) / 2;

    int cancelBoxX = optionsStartX;
    int confirmBoxX = optionsStartX + optionBoxWidth + 10;


    // Draw Cancel (X)
    if (disconnectOverlaySelection == 0) {  // Highlight Cancel
      u8g2.setDrawColor(0);                 // Black filled box for selection
      u8g2.drawRBox(cancelBoxX, optionBoxY, optionBoxWidth, optionBoxHeight, 2);
      u8g2.setDrawColor(1);  // White text on black selection
      u8g2.drawStr(cancelBoxX + (optionBoxWidth - cancelWidth) / 2, optionTextY, cancelText);
    } else {
      u8g2.setDrawColor(0);  // Black frame for non-selected
      u8g2.drawRFrame(cancelBoxX, optionBoxY, optionBoxWidth, optionBoxHeight, 2);
      u8g2.drawStr(cancelBoxX + (optionBoxWidth - cancelWidth) / 2, optionTextY, cancelText);
    }

    // Draw Confirm (O)
    u8g2.setDrawColor(0);                   // Ensure black for elements on white bg before checking selection
    if (disconnectOverlaySelection == 1) {  // Highlight Confirm
      u8g2.drawRBox(confirmBoxX, optionBoxY, optionBoxWidth, optionBoxHeight, 2);
      u8g2.setDrawColor(1);  // White text on black selection
      u8g2.drawStr(confirmBoxX + (optionBoxWidth - confirmWidth) / 2, optionTextY, confirmText);
    } else {
      u8g2.drawRFrame(confirmBoxX, optionBoxY, optionBoxWidth, optionBoxHeight, 2);
      u8g2.drawStr(confirmBoxX + (optionBoxWidth - confirmWidth) / 2, optionTextY, confirmText);
    }
  }

  // Reset draw color to white for elements outside the overlay
  u8g2.setDrawColor(1);
}

void drawJammingActiveScreen() {
  u8g2.setFont(u8g2_font_7x13B_tr);  // Larger font for active status
  u8g2.setDrawColor(1);

  const char* jamStatusText = "Jamming Initializing...";
  switch (activeJammingType) {
    case JAM_BLE:
      jamStatusText = "BLE Jam Active";
      break;
    case JAM_BT:
      jamStatusText = "BT Jam Active";
      break;
    case JAM_NRF_CHANNELS:
      jamStatusText = "NRF Channel Jam";
      break;
    case JAM_RF_NOISE_FLOOD:
      jamStatusText = "RF Noise Flood";
      break;
    default:
      jamStatusText = "Jammer Idle";  // Should not happen if screen is active
      break;
  }

  int textWidth = u8g2.getStrWidth(jamStatusText);
  int textX = (u8g2.getDisplayWidth() - textWidth) / 2;
  int textY = u8g2.getDisplayHeight() / 2 - 5;  // Centered vertically

  u8g2.drawStr(textX, textY, jamStatusText);

  u8g2.setFont(u8g2_font_5x7_tf);
  const char* instruction = "Hold BACK to Stop";
  textWidth = u8g2.getStrWidth(instruction);
  textX = (u8g2.getDisplayWidth() - textWidth) / 2;
  textY = u8g2.getDisplayHeight() - 15;
  u8g2.drawStr(textX, textY, instruction);

  // No graph or visualizer, as per request.
}

// NEW Function: drawOtaStatusScreen
void drawOtaStatusScreen() {
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setDrawColor(1);

  const char* title = "Firmware Update";
  if (currentMenu == OTA_WEB_ACTIVE) title = "Web Update";
  else if (currentMenu == OTA_SD_STATUS) title = "SD Card Update";
  else if (currentMenu == OTA_BASIC_ACTIVE) title = "Basic OTA (IDE)";

  // Calculate vertical starting position dynamically
  // Start drawing below the status bar if it's drawn, otherwise near the top
  int currentY = STATUS_BAR_H + 6;  // Default starting Y if status bar is present
  // If status bar is not drawn for these screens, adjust:
  bool statusBarWasDrawn = !(currentMenu == WIFI_PASSWORD_INPUT || currentMenu == WIFI_CONNECTING || currentMenu == WIFI_CONNECTION_INFO || currentMenu == JAMMING_ACTIVE_SCREEN || currentMenu == OTA_WEB_ACTIVE || currentMenu == OTA_SD_STATUS || currentMenu == OTA_BASIC_ACTIVE);
  if (!statusBarWasDrawn) {
    currentY = 8;  // Closer to top if no status bar
  }


  int titleWidth = u8g2.getStrWidth(title);
  u8g2.drawStr((u8g2.getDisplayWidth() - titleWidth) / 2, currentY, title);
  currentY += 12;  // Space after title

  if ((currentMenu == OTA_WEB_ACTIVE || currentMenu == OTA_BASIC_ACTIVE) && strlen(otaDisplayIpAddress) > 0) {
    int ipWidth = u8g2.getStrWidth(otaDisplayIpAddress);
    u8g2.drawStr((u8g2.getDisplayWidth() - ipWidth) / 2, currentY, otaDisplayIpAddress);
    currentY += 12;  // Space after IP
  }

  if (otaStatusMessage.length() > 0) {
    String line1 = "", line2 = "";
    int maxWidthChars = (u8g2.getDisplayWidth() - 8) / u8g2.getMaxCharWidth();  // Approx chars that fit
    if (otaStatusMessage.length() > maxWidthChars) {
      int breakPoint = -1;
      // Try to break at a space or hyphen for better readability
      for (int i = maxWidthChars; i > 0; --i) {
        if (otaStatusMessage.charAt(i) == ' ' || otaStatusMessage.charAt(i) == '-' || otaStatusMessage.charAt(i) == ':') {
          breakPoint = i;
          break;
        }
      }
      if (breakPoint != -1 && breakPoint > 0) {                 // Ensure breakpoint is valid
        line1 = otaStatusMessage.substring(0, breakPoint + 1);  // Include the space/hyphen if preferred
        line2 = otaStatusMessage.substring(breakPoint + 1);
        line2.trim();  // Remove leading space from line2 if break char was space
      } else {         // Hard break if no good spot found
        line1 = otaStatusMessage.substring(0, maxWidthChars);
        line2 = otaStatusMessage.substring(maxWidthChars);
      }
    } else {
      line1 = otaStatusMessage;
    }

    int status1Width = u8g2.getStrWidth(line1.c_str());
    u8g2.drawStr((u8g2.getDisplayWidth() - status1Width) / 2, currentY, line1.c_str());
    if (line2.length() > 0) {
      currentY += 10;  // Space for second line of status
      int status2Width = u8g2.getStrWidth(line2.c_str());
      u8g2.drawStr((u8g2.getDisplayWidth() - status2Width) / 2, currentY, line2.c_str());
    }
    currentY += 12;  // Extra space after status message before potential progress bar
  }

  // Determine if progress bar should be drawn
  bool shouldDrawProgressBar = false;
  if (currentMenu == OTA_WEB_ACTIVE) {
    // For Web OTA, only show progress if upload has actually started (otaProgress > 0)
    // or if it's completed and rebooting. otaProgress = 0 means server is up, waiting.
    if (otaProgress > 0 && otaProgress <= 100) {
      shouldDrawProgressBar = true;
    }
  } else if (currentMenu == OTA_SD_STATUS || currentMenu == OTA_BASIC_ACTIVE) {
    // For SD and Basic, progress 0 is a valid starting point of the process.
    if (otaProgress >= 0 && otaProgress <= 100) {
      shouldDrawProgressBar = true;
    }
  }

  // Position for bottom elements (progress bar or error text)
  int bottomElementY = u8g2.getDisplayHeight() - 15;  // Y for top of bar/error text
  int barH = 7;

  // Adjust bottomElementY if text content (currentY) is too close or overlaps
  if (currentY > bottomElementY - barH - 2) {
    bottomElementY = currentY + 2;  // Move bar below text
    // Ensure bar still fits on screen
    if (bottomElementY + barH > u8g2.getDisplayHeight() - 2) {
      bottomElementY = u8g2.getDisplayHeight() - barH - 2;
    }
  }


  if (shouldDrawProgressBar) {
    int barX = 10;
    int barW = u8g2.getDisplayWidth() - 20;
    u8g2.drawFrame(barX, bottomElementY, barW, barH);
    int fillW = (barW - 2) * otaProgress / 100;
    // Clamp fillW to prevent underflow/overflow visually
    if (fillW < 0) fillW = 0;
    if (fillW > barW - 2) fillW = barW - 2;

    if (fillW > 0) {
      u8g2.drawBox(barX + 1, bottomElementY + 1, fillW, barH - 2);
    }
  } else if (otaProgress == -1) {              // Error occurred
    const char* errorText = "Error Occurred";  // Generic, otaStatusMessage should have details
    int errorTextWidth = u8g2.getStrWidth(errorText);
    // Draw error text centered vertically in the space normally for the bar
    u8g2.drawStr((u8g2.getDisplayWidth() - errorTextWidth) / 2, bottomElementY + (barH - (u8g2.getAscent() - u8g2.getDescent())) / 2 + u8g2.getAscent(), errorText);
  }
  // If otaProgress is -2 (idle/default) or -3 (already up-to-date for web), no bar or error text is drawn for the bar area.
  // The otaStatusMessage itself will convey "Already up to date".
}

// NEW Function: drawSDFirmwareListScreen
void drawSDFirmwareListScreen() {
  const int list_start_y_abs = STATUS_BAR_H + 1;
  const int list_visible_height = u8g2.getDisplayHeight() - list_start_y_abs;
  const int item_row_h = FW_LIST_ITEM_H;  // Use FW_LIST_ITEM_H
  const int item_padding_x = 2;
  const int content_padding_x = 4;

  u8g2.setClipWindow(0, list_start_y_abs, u8g2.getDisplayWidth(), u8g2.getDisplayHeight());

  // Scroll animation logic (reusing wifiListAnim variables and logic)
  float target_scroll_for_centering = 0;
  if (maxMenuItems > 0 && (maxMenuItems * item_row_h > list_visible_height)) {
    target_scroll_for_centering = (wifiMenuIndex * item_row_h) - (list_visible_height / 2) + (item_row_h / 2);
    if (target_scroll_for_centering < 0) target_scroll_for_centering = 0;
    float max_scroll_val = (maxMenuItems * item_row_h) - list_visible_height;
    if (max_scroll_val < 0) max_scroll_val = 0;
    if (target_scroll_for_centering > max_scroll_val) {
      target_scroll_for_centering = max_scroll_val;
    }
  }
  float scrollDiff = target_scroll_for_centering - currentWifiListScrollOffset_Y_anim;
  if (abs(scrollDiff) > 0.1f) {
    currentWifiListScrollOffset_Y_anim += scrollDiff * GRID_ANIM_SPEED * 0.016f;
  } else {
    currentWifiListScrollOffset_Y_anim = target_scroll_for_centering;
  }

  wifiListAnim.update();  // Reusing wifiListAnim for firmware list items
  u8g2.setFont(u8g2_font_6x10_tf);

  if (availableSdFirmwareCount == 0) {
    u8g2.setDrawColor(1);
    // otaStatusMessage should be "No firmwares found..." from prepareSdFirmwareList
    const char* noFwMsg = otaStatusMessage.c_str();
    if (strlen(noFwMsg) == 0) noFwMsg = "No Firmware Files";  // Fallback

    int msgWidth = u8g2.getStrWidth(noFwMsg);
    int text_y_pos = list_start_y_abs + (list_visible_height - (u8g2.getAscent() - u8g2.getDescent())) / 2 + u8g2.getAscent();
    u8g2.drawStr((u8g2.getDisplayWidth() - msgWidth) / 2, text_y_pos, noFwMsg);
  }

  for (int i = 0; i < maxMenuItems; i++) {  // maxMenuItems includes "Back"
    int item_center_y_in_full_list = (i * item_row_h) + (item_row_h / 2);
    int item_center_on_screen_y = list_start_y_abs + item_center_y_in_full_list - (int)currentWifiListScrollOffset_Y_anim;
    int item_top_on_screen_y = item_center_on_screen_y - item_row_h / 2;

    if (item_top_on_screen_y + item_row_h < list_start_y_abs || item_top_on_screen_y >= u8g2.getDisplayHeight()) {
      continue;
    }

    bool is_selected_item = (i == wifiMenuIndex);  // wifiMenuIndex is used for list selection

    if (is_selected_item) {
      u8g2.setDrawColor(1);
      drawRndBox(item_padding_x, item_top_on_screen_y, u8g2.getDisplayWidth() - 2 * item_padding_x, item_row_h, 2, true);
      u8g2.setDrawColor(0);
    } else {
      u8g2.setDrawColor(1);
    }

    String displayText;
    int action_icon_type = -1;  // For "Back" item

    if (i < availableSdFirmwareCount) {
      // Display FirmwareInfo: version and maybe build_date or description
      displayText = availableSdFirmwares[i].version;
      if (strlen(availableSdFirmwares[i].description) > 0 && strlen(availableSdFirmwares[i].version) == 0) {
        displayText = availableSdFirmwares[i].description;  // Fallback to description if version is empty
      } else if (strlen(availableSdFirmwares[i].version) == 0 && strlen(availableSdFirmwares[i].description) == 0) {
        displayText = availableSdFirmwares[i].binary_filename;  // Fallback to filename
      }
      // Optionally append build date if space allows, or use marquee
      // For simplicity, just version for now
      action_icon_type = 15;  // System Info Icon (as a generic FW icon)
    } else {                  // This is the "Back" item
      displayText = "Back";
      action_icon_type = 8;  // Back arrow icon
    }

    if (displayText.isEmpty()) continue;

    int current_content_x = content_padding_x + item_padding_x;
    int icon_y_center_for_draw = item_center_on_screen_y;

    if (action_icon_type != -1) {
      drawCustomIcon(current_content_x, icon_y_center_for_draw - 4, action_icon_type, false);  // Small icon
      current_content_x += 10;                                                                 // Space for icon
    } else {
      current_content_x += 10;  // Default padding if no icon
    }

    int text_x_start = current_content_x;
    int text_available_width = u8g2.getDisplayWidth() - text_x_start - content_padding_x - item_padding_x;
    int text_baseline_y = item_center_on_screen_y - (u8g2.getAscent() + u8g2.getDescent()) / 2 + u8g2.getAscent();

    if (is_selected_item) {
      updateMarquee(text_available_width, displayText.c_str());
      if (marqueeActive) {
        u8g2.drawStr(text_x_start + (int)marqueeOffset, text_baseline_y, marqueeText);
      } else {
        char* truncated = truncateText(displayText.c_str(), text_available_width, u8g2);
        u8g2.drawStr(text_x_start, text_baseline_y, truncated);
      }
    } else {
      char* truncated = truncateText(displayText.c_str(), text_available_width, u8g2);
      u8g2.drawStr(text_x_start, text_baseline_y, truncated);
    }
  }
  u8g2.setDrawColor(1);
  u8g2.setMaxClipWindow();
}

void drawUI() {
  unsigned long currentTime = millis();  // Ensure currentTime is available for animations

  if (currentMenu == FLASHLIGHT_MODE) {
    selectMux(MUX_CHANNEL_MAIN_DISPLAY);
    u8g2.clearBuffer();
    u8g2.setDrawColor(1);
    u8g2.drawBox(0, 0, u8g2.getDisplayWidth(), u8g2.getDisplayHeight());
    u8g2.sendBuffer();
    selectMux(MUX_CHANNEL_SECOND_DISPLAY);
    u8g2_small.clearBuffer();
    u8g2_small.setDrawColor(1);
    u8g2_small.drawBox(0, 0, u8g2_small.getDisplayWidth(), u8g2_small.getDisplayHeight());
    u8g2_small.sendBuffer();
    return;
  }

  selectMux(MUX_CHANNEL_SECOND_DISPLAY);
  drawPassiveDisplay();

  selectMux(MUX_CHANNEL_MAIN_DISPLAY);
  u8g2.clearBuffer();

  bool drawDefaultStatusBar = true;
  if (currentMenu == WIFI_PASSWORD_INPUT || currentMenu == WIFI_CONNECTING || currentMenu == WIFI_CONNECTION_INFO || currentMenu == JAMMING_ACTIVE_SCREEN || currentMenu == OTA_WEB_ACTIVE || currentMenu == OTA_SD_STATUS || currentMenu == OTA_BASIC_ACTIVE) {
    drawDefaultStatusBar = false;
  }
  if (drawDefaultStatusBar) { drawStatusBar(); }

  if (currentMenu == JAMMING_ACTIVE_SCREEN) {
    drawJammingActiveScreen();
  } else if (currentMenu == TOOL_CATEGORY_GRID || currentMenu == FIRMWARE_UPDATE_GRID) {  // Combined grid handling
    // Scroll Animation for Grid
    float scrollDiff = targetGridScrollOffset_Y - currentGridScrollOffset_Y_anim;
    if (abs(scrollDiff) > 0.1f) {
      currentGridScrollOffset_Y_anim += scrollDiff * GRID_ANIM_SPEED * 0.016f;
      if ((scrollDiff > 0 && currentGridScrollOffset_Y_anim > targetGridScrollOffset_Y) || (scrollDiff < 0 && currentGridScrollOffset_Y_anim < targetGridScrollOffset_Y)) {
        currentGridScrollOffset_Y_anim = targetGridScrollOffset_Y;
      }
    } else {
      currentGridScrollOffset_Y_anim = targetGridScrollOffset_Y;
    }

    // Item Scale Animation for Grid
    if (gridAnimatingIn) {
      bool stillAnimating = false;
      int currentGridItemCount = maxMenuItems;
      for (int i = 0; i < currentGridItemCount && i < MAX_GRID_ITEMS; ++i) {
        if (currentTime >= gridItemAnimStartTime[i]) {
          float targetScaleForThisItem = gridItemTargetScale[i];
          float diffScale = targetScaleForThisItem - gridItemScale[i];
          if (abs(diffScale) > 0.01f) {
            gridItemScale[i] += diffScale * GRID_ANIM_SPEED * 0.016f;
            if ((diffScale > 0 && gridItemScale[i] > targetScaleForThisItem) || (diffScale < 0 && gridItemScale[i] < targetScaleForThisItem)) {
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
    drawToolGridScreen();
  } else if (currentMenu == MAIN_MENU) {
    drawMainMenu();
  } else if (currentMenu == WIFI_SETUP_MENU) {
    drawWifiSetupScreen();
  } else if (currentMenu == WIFI_PASSWORD_INPUT) {
    drawPasswordInputScreen();
  } else if (currentMenu == WIFI_CONNECTING) {
    drawWifiConnectingStatusScreen();
  } else if (currentMenu == WIFI_CONNECTION_INFO) {
    drawWifiConnectionResultScreen();
  } else if (currentMenu == OTA_WEB_ACTIVE || currentMenu == OTA_SD_STATUS || currentMenu == OTA_BASIC_ACTIVE) {
    drawOtaStatusScreen();
  } else if (currentMenu == FIRMWARE_SD_LIST_MENU) {
    drawSDFirmwareListScreen();
  } else {
    if (currentMenu == GAMES_MENU || currentMenu == TOOLS_MENU || currentMenu == SETTINGS_MENU || currentMenu == UTILITIES_MENU) {
      drawCarouselMenu();
    } else {
      u8g2.setFont(u8g2_font_6x10_tf);
      u8g2.setDrawColor(1);
      u8g2.drawStr(5, 30, "Render Error:");
      char menuNumStr[10];
      sprintf(menuNumStr, "Menu %d", currentMenu);
      u8g2.drawStr(5, 42, menuNumStr);
      Serial.printf("UI_DRAWING: Unhandled menu state %d in drawUI(), drawing placeholder.\n", currentMenu);
    }
  }

  // Draw overlay on top if active
  // PHASE 2: Add || showWifiRedirectPromptOverlay
  if (showWifiDisconnectOverlay) {
    drawWifiDisconnectOverlay();
  }

  u8g2.sendBuffer();
}

void drawPassiveDisplay() {
  if (currentMenu == WIFI_PASSWORD_INPUT) {
    drawKeyboardOnSmallDisplay();
  } else {
    u8g2_small.clearBuffer();
    u8g2_small.setDrawColor(1);

    // REMOVED UPTIME CLOCK
    // unsigned long now = millis(); unsigned long allSeconds = now / 1000;
    // int runHours = allSeconds / 3600; int secsRemaining = allSeconds % 3600;
    // int runMinutes = secsRemaining / 60; int runSeconds = secsRemaining % 60;
    // char timeStr[9]; sprintf(timeStr, "%02d:%02d:%02d", runHours, runMinutes, runSeconds);
    // u8g2_small.setFont(u8g2_font_helvB08_tr);
    // int time_y_baseline = 17;
    // u8g2_small.drawStr((u8g2_small.getDisplayWidth() - u8g2_small.getStrWidth(timeStr)) / 2, time_y_baseline, timeStr);

    // Battery Info Section (moved up slightly since time is removed)
    float v_bat = currentBatteryVoltage;  // Use the globally updated smooth voltage
    uint8_t s_bat = batPerc(v_bat);
    char batInfoStr[15];
    snprintf(batInfoStr, sizeof(batInfoStr), "%.2fV %s%d%%", v_bat, isCharging ? "+" : "", s_bat);
    u8g2_small.setFont(u8g2_font_5x7_tf);
    int bat_info_y = u8g2_small.getAscent() + 2;  // Top part of display
    u8g2_small.drawStr((u8g2_small.getDisplayWidth() - u8g2_small.getStrWidth(batInfoStr)) / 2, bat_info_y, batInfoStr);


    const char* modeText = "---";
    if (isJammingOperationActive) {  // <--- NEW: Show jamming status
      switch (activeJammingType) {
        case JAM_BLE: modeText = "BLE Jamming"; break;
        case JAM_BT: modeText = "BT Jamming"; break;
        case JAM_NRF_CHANNELS: modeText = "NRF Jamming"; break;
        case JAM_RF_NOISE_FLOOD: modeText = "RF Flooding"; break;
        default: modeText = "Jammer Active"; break;
      }
    } else if (showWifiDisconnectOverlay) {
      modeText = "Disconnecting...";
      // ... (rest of modeText logic unchanged for other Kiva states) ...
    } else if (currentMenu == FLASHLIGHT_MODE) modeText = "Torch";
    else if (currentMenu == MAIN_MENU && menuIndex < getMainMenuItemsCount()) modeText = mainMenuItems[menuIndex];
    else if (currentMenu == UTILITIES_MENU && menuIndex < getUtilitiesMenuItemsCount()) modeText = utilitiesMenuItems[menuIndex];
    else if (currentMenu == GAMES_MENU && menuIndex < getGamesMenuItemsCount() && strcmp(gamesMenuItems[menuIndex], "Back") != 0) modeText = gamesMenuItems[menuIndex];
    else if (currentMenu == TOOLS_MENU && menuIndex < getToolsMenuItemsCount() && strcmp(toolsMenuItems[menuIndex], "Back") != 0) modeText = toolsMenuItems[menuIndex];
    else if (currentMenu == SETTINGS_MENU && menuIndex < getSettingsMenuItemsCount() && strcmp(settingsMenuItems[menuIndex], "Back") != 0) modeText = settingsMenuItems[menuIndex];
    else if (currentMenu == TOOL_CATEGORY_GRID && toolsCategoryIndex < (getToolsMenuItemsCount() - 1)) {
      // Check if it's the "Jamming" category
      if (strcmp(toolsMenuItems[toolsCategoryIndex], "Jamming") == 0) {
        if (menuIndex < getJammingToolItemsCount() && strcmp(jammingToolItems[menuIndex], "Back") != 0) {
          modeText = jammingToolItems[menuIndex];
        } else {
          modeText = "Jamming Tools";
        }
      } else {  // Other tool categories
        modeText = toolsMenuItems[toolsCategoryIndex];
      }
    } else if (currentMenu == WIFI_SETUP_MENU) {
      if (wifiIsScanning) modeText = "Scanning WiFi";
      else modeText = "Wi-Fi Setup";
    } else if (currentMenu == WIFI_CONNECTING) {
      modeText = "Connecting...";
    } else if (currentMenu == WIFI_CONNECTION_INFO) {
      modeText = getWifiStatusMessage();
    }


    u8g2_small.setFont(u8g2_font_6x12_tr);  // Slightly larger font for mode text
    char truncatedModeText[22];
    strncpy(truncatedModeText, modeText, sizeof(truncatedModeText) - 1);
    truncatedModeText[sizeof(truncatedModeText) - 1] = '\0';
    if (u8g2_small.getStrWidth(truncatedModeText) > u8g2_small.getDisplayWidth() - 4) {
      truncateText(modeText, u8g2_small.getDisplayWidth() - 4, u8g2_small);  // SBUF is used here
      strncpy(truncatedModeText, SBUF, sizeof(truncatedModeText) - 1);
      truncatedModeText[sizeof(truncatedModeText) - 1] = '\0';
    }
    int mode_text_y_baseline = u8g2_small.getDisplayHeight() - 2;  // Bottom part of display
    u8g2_small.drawStr((u8g2_small.getDisplayWidth() - u8g2_small.getStrWidth(truncatedModeText)) / 2, mode_text_y_baseline, truncatedModeText);

    u8g2_small.sendBuffer();
  }
}



void updateMarquee(int cardInnerW, const char* text) {
  if (text == nullptr || strlen(text) == 0) {
    marqueeActive = false;
    marqueeText[0] = '\0';
    return;
  }

  bool shouldBeActive = u8g2.getUTF8Width(text) > cardInnerW;

  if (!marqueeActive && shouldBeActive) {
    strncpy(marqueeText, text, sizeof(marqueeText) - 1);
    marqueeText[sizeof(marqueeText) - 1] = '\0';
    marqueeTextLenPx = u8g2.getUTF8Width(marqueeText);
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
    if (strncmp(marqueeText, text, sizeof(marqueeText) - 1) != 0) {
      strncpy(marqueeText, text, sizeof(marqueeText) - 1);
      marqueeText[sizeof(marqueeText) - 1] = '\0';
      marqueeTextLenPx = u8g2.getUTF8Width(marqueeText);
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

  if (curT - lastMarqueeTime > MARQUEE_UPDATE_INTERVAL) {
    if (marqueeScrollLeft) {
      marqueeOffset -= MARQUEE_SCROLL_SPEED;
    } else {
      marqueeOffset += MARQUEE_SCROLL_SPEED;
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

char* truncateText(const char* originalText, int maxWidthPixels, U8G2& display) {
  int originalLen = strlen(originalText);
  if (originalLen == 0) {
    SBUF[0] = '\0';
    return SBUF;
  }

  int maxTextPartLen = sizeof(SBUF) - 4;
  if (maxTextPartLen < 1) {
    SBUF[0] = '\0';
    return SBUF;
  }

  strncpy(SBUF, originalText, maxTextPartLen);
  SBUF[maxTextPartLen] = '\0';

  if (display.getUTF8Width(SBUF) <= maxWidthPixels) {
    if (originalLen <= maxTextPartLen) {
      strncpy(SBUF, originalText, sizeof(SBUF) - 1);
      SBUF[sizeof(SBUF) - 1] = '\0';
      return SBUF;
    }
    return SBUF;
  }

  strcat(SBUF, "...");

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

void drawStatusBar() {
  float v = getSmoothV();
  uint8_t s = batPerc(v);
  u8g2.setFont(u8g2_font_6x10_tf);
  const char* titleText = "Kiva";

  if (currentMenu == TOOL_CATEGORY_GRID) {
    if (toolsCategoryIndex >= 0 && toolsCategoryIndex < (getToolsMenuItemsCount() - 1)) {
      titleText = toolsMenuItems[toolsCategoryIndex];
    } else {
      titleText = "Tools";
    }
  } else if (currentMenu == WIFI_SETUP_MENU) {
    titleText = "Wi-Fi Setup";
  } else if (currentMenu == FIRMWARE_UPDATE_GRID) {  // <--- NEW
    titleText = "Firmware Update";
  } else if (currentMenu == FIRMWARE_SD_LIST_MENU) {
    titleText = "Update from SD";
  }  // <--- NEW TITLE
  else if (currentMenu != MAIN_MENU && mainMenuSavedIndex >= 0 && mainMenuSavedIndex < getMainMenuItemsCount()) {
    titleText = mainMenuItems[mainMenuSavedIndex];
  }

  char titleBuffer[14];
  strncpy(titleBuffer, titleText, sizeof(titleBuffer) - 1);
  titleBuffer[sizeof(titleBuffer) - 1] = '\0';
  int max_title_width = 65;
  if (u8g2.getStrWidth(titleBuffer) > max_title_width) {
    truncateText(titleText, max_title_width - 5, u8g2);
    strncpy(titleBuffer, SBUF, sizeof(titleBuffer) - 1);
    titleBuffer[sizeof(titleBuffer) - 1] = '\0';
  }
  u8g2.drawStr(2, 7, titleBuffer);

  int battery_area_right_margin = 2;
  int current_x_origin_for_battery = u8g2.getDisplayWidth() - battery_area_right_margin;
  int bat_icon_y_top = ((STATUS_BAR_H - 5) / 2) - 2;
  if (bat_icon_y_top < 0) bat_icon_y_top = 0;
  int bat_icon_width = 9;
  current_x_origin_for_battery -= bat_icon_width;
  drawBatIcon(current_x_origin_for_battery, bat_icon_y_top, s);

  if (isCharging) {
    int charge_icon_width = 6;
    int charge_icon_height = 7;
    int charge_icon_spacing = 2;
    current_x_origin_for_battery -= (charge_icon_width + charge_icon_spacing);
    int charge_icon_y_top = ((STATUS_BAR_H - charge_icon_height) / 2) - 2;
    if (charge_icon_y_top < 0) charge_icon_y_top = 0;
    u8g2.setDrawColor(1);
    drawCustomIcon(current_x_origin_for_battery, charge_icon_y_top, 17, false);
  }

  char batteryStr[6];
  snprintf(batteryStr, sizeof(batteryStr), "%u%%", s);
  int percent_text_width = u8g2.getStrWidth(batteryStr);
  int percent_text_spacing = 2;
  current_x_origin_for_battery -= (percent_text_width + percent_text_spacing);
  u8g2.setDrawColor(1);
  u8g2.drawStr(current_x_origin_for_battery, 7, batteryStr);

  u8g2.setDrawColor(1);
  u8g2.drawLine(0, STATUS_BAR_H - 1, u8g2.getDisplayWidth() - 1, STATUS_BAR_H - 1);
}


void drawMainMenu() {
  mainMenuAnim.update();
  const char** itms = mainMenuItems;
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

    int iconType = 0;
    if (strcmp(itms[i], "Tools") == 0) iconType = 0;
    else if (strcmp(itms[i], "Games") == 0) iconType = 1;
    else if (strcmp(itms[i], "Settings") == 0) iconType = 2;
    else if (strcmp(itms[i], "Utilities") == 0) iconType = 21;
    else if (strcmp(itms[i], "Info") == 0) iconType = 3;

    drawCustomIcon(icon_x_pos, icon_y_pos, iconType, true);

    u8g2.setFont(u8g2_font_6x10_tf);
    int text_width_pixels = u8g2.getStrWidth(itms[i]);
    int text_x_render_pos = text_box_x_pos + (text_box_w_scaled - text_width_pixels) / 2;
    int text_y_baseline_render_pos = item_abs_center_y + 4;

    if (text_box_w_scaled > 0 && text_box_h_scaled > 0) {
      if (is_selected_item) {
        u8g2.setDrawColor(1);
        drawRndBox(text_box_x_pos, text_box_y_pos, text_box_w_scaled, text_box_h_scaled, 2, true);
        u8g2.setDrawColor(0);
        u8g2.drawStr(text_x_render_pos, text_y_baseline_render_pos, itms[i]);
      } else {
        u8g2.setDrawColor(1);
        drawRndBox(text_box_x_pos, text_box_y_pos, text_box_w_scaled, text_box_h_scaled, 2, false);
        u8g2.drawStr(text_x_render_pos, text_y_baseline_render_pos, itms[i]);
      }
    }
  }
  u8g2.setDrawColor(1);
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

  u8g2.setFont(u8g2_font_6x10_tf);

  for (int i = 0; i < maxMenuItems; i++) {
    float current_item_scale = subMenuAnim.itemScale[i];
    if (current_item_scale <= 0.05f) continue;

    float current_item_offset_x = subMenuAnim.itemOffsetX[i];
    int card_w_scaled = (int)(CarouselAnimation().cardBaseW * current_item_scale);
    int card_h_scaled = (int)(CarouselAnimation().cardBaseH * current_item_scale);
    int card_x_abs = screen_center_x_abs + (int)current_item_offset_x - (card_w_scaled / 2);
    int card_y_abs = carousel_center_y_abs - (card_h_scaled / 2);
    bool is_selected_item = (i == menuIndex);

    const char** current_item_list_ptr = nullptr;
    int icon_type_for_item = 0;

    char itemDisplayTextBuffer[25];
    const char* itemTextToDisplay = nullptr;

    if (currentMenu == GAMES_MENU) {
      current_item_list_ptr = gamesMenuItems;
      itemTextToDisplay = current_item_list_ptr[i];
      if (i == 0) icon_type_for_item = 4;
      else if (i == 1) icon_type_for_item = 5;
      else if (i == 2) icon_type_for_item = 6;
      else if (i == 3) icon_type_for_item = 7;
      else if (i == 4 && i < getGamesMenuItemsCount()) icon_type_for_item = 8;
      else continue;

    } else if (currentMenu == TOOLS_MENU) {
      current_item_list_ptr = toolsMenuItems;
      itemTextToDisplay = current_item_list_ptr[i];
      if (strcmp(itemTextToDisplay, "Injection") == 0) icon_type_for_item = 12;
      else if (strcmp(itemTextToDisplay, "Wi-Fi Attacks") == 0) icon_type_for_item = 9;
      else if (strcmp(itemTextToDisplay, "BLE/BT Attacks") == 0) icon_type_for_item = 10;
      else if (strcmp(itemTextToDisplay, "NRF Recon") == 0) icon_type_for_item = 15;  // Using System Info icon for recon
      else if (strcmp(itemTextToDisplay, "Jamming") == 0) icon_type_for_item = 11;    // Jamming icon
      else if (strcmp(itemTextToDisplay, "Back") == 0) icon_type_for_item = 8;
      else continue;
    } else if (currentMenu == SETTINGS_MENU) {
      current_item_list_ptr = settingsMenuItems;
      itemTextToDisplay = current_item_list_ptr[i];
      if (i == 0) icon_type_for_item = 9;
      else if (i == 1) icon_type_for_item = 13;
      else if (i == 2) icon_type_for_item = 14;
      else if (i == 3) icon_type_for_item = 15;
      else if (i == 4) icon_type_for_item = 3;
      else if (i == 5 && i < getSettingsMenuItemsCount()) icon_type_for_item = 8;
      else continue;

    } else if (currentMenu == UTILITIES_MENU) {
      current_item_list_ptr = utilitiesMenuItems;
      itemTextToDisplay = current_item_list_ptr[i];

      if (strcmp(itemTextToDisplay, "Vibration") == 0) {
        icon_type_for_item = 18;
        snprintf(itemDisplayTextBuffer, sizeof(itemDisplayTextBuffer), "Vibration: %s", vibrationOn ? "ON" : "OFF");
        itemTextToDisplay = itemDisplayTextBuffer;
      } else if (strcmp(itemTextToDisplay, "Laser") == 0) {
        icon_type_for_item = 19;
        snprintf(itemDisplayTextBuffer, sizeof(itemDisplayTextBuffer), "Laser: %s", laserOn ? "ON" : "OFF");
        itemTextToDisplay = itemDisplayTextBuffer;
      } else if (strcmp(itemTextToDisplay, "Flashlight") == 0) {
        icon_type_for_item = 20;
      } else if (strcmp(itemTextToDisplay, "Back") == 0) {
        icon_type_for_item = 8;
      } else {
        continue;
      }
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
      drawCustomIcon(icon_x_pos, icon_y_pos, icon_type_for_item, true);

      int text_area_y_start_abs = card_y_abs + icon_margin_top + icon_render_size + text_margin_from_icon;
      int text_area_h_available = card_h_scaled - (text_area_y_start_abs - card_y_abs) - card_internal_padding;
      int text_baseline_y_render = text_area_y_start_abs + (text_area_h_available - (u8g2.getAscent() - u8g2.getDescent())) / 2 + u8g2.getAscent() - 1;


      int card_inner_content_width = card_w_scaled - 2 * card_internal_padding;
      updateMarquee(card_inner_content_width, itemTextToDisplay);

      int clip_x1 = card_x_abs + card_internal_padding;
      int clip_y1 = text_area_y_start_abs;
      int clip_x2 = card_x_abs + card_w_scaled - card_internal_padding;
      int clip_y2 = card_y_abs + card_h_scaled - card_internal_padding;

      clip_y1 = max(clip_y1, card_y_abs);
      clip_y2 = min(clip_y2, card_y_abs + card_h_scaled);

      if (clip_x1 < clip_x2 && clip_y1 < clip_y2) {
        u8g2.setClipWindow(clip_x1, clip_y1, clip_x2, clip_y2);
        if (marqueeActive) {
          u8g2.drawStr(card_x_abs + card_internal_padding + (int)marqueeOffset, text_baseline_y_render, marqueeText);
        } else {
          int text_width_pixels = u8g2.getStrWidth(itemTextToDisplay);
          u8g2.drawStr(card_x_abs + (card_w_scaled - text_width_pixels) / 2, text_baseline_y_render, itemTextToDisplay);
        }
        u8g2.setMaxClipWindow();
      }
    } else {
      u8g2.setDrawColor(1);
      drawRndBox(card_x_abs, card_y_abs, card_w_scaled, card_h_scaled, 2, false);

      if (current_item_scale > 0.5) {
        u8g2.setFont(u8g2_font_5x7_tf);
        char* display_text_truncated = truncateText(itemTextToDisplay, card_w_scaled - 2 * card_internal_padding, u8g2);
        int text_width_pixels = u8g2.getStrWidth(display_text_truncated);
        int text_baseline_y_render = card_y_abs + (card_h_scaled - (u8g2.getAscent() - u8g2.getDescent())) / 2 + u8g2.getAscent();
        u8g2.drawStr(card_x_abs + (card_w_scaled - text_width_pixels) / 2, text_baseline_y_render, display_text_truncated);
        u8g2.setFont(u8g2_font_6x10_tf);
      }
    }
  }
  u8g2.setDrawColor(1);
}


void startGridItemAnimation() {
  gridAnimatingIn = true;
  unsigned long currentTime = millis();
  int currentGridItemCount = maxMenuItems;

  for (int i = 0; i < MAX_GRID_ITEMS; i++) {
    if (i < currentGridItemCount) {
      gridItemScale[i] = 0.0f;
      gridItemTargetScale[i] = 1.0f;
      int row = i / gridCols;
      int col = i % gridCols;
      gridItemAnimStartTime[i] = currentTime + (unsigned long)((row * 1.5f + col) * GRID_ANIM_STAGGER_DELAY);
    } else {
      gridItemScale[i] = 0.0f;
      gridItemTargetScale[i] = 0.0f;
      gridItemAnimStartTime[i] = currentTime;
    }
  }
}

void drawToolGridScreen() {
  const char** current_item_list_ptr = nullptr;  // <--- DECLARE THE VARIABLE HERE

  if (currentMenu == TOOL_CATEGORY_GRID) {
    switch (toolsCategoryIndex) {
      case 0: current_item_list_ptr = injectionToolItems; break;
      case 1: current_item_list_ptr = wifiAttackToolItems; break;
      case 2: current_item_list_ptr = bleAttackToolItems; break;
      case 3: current_item_list_ptr = nrfReconToolItems; break;
      case 4: current_item_list_ptr = jammingToolItems; break;
      default: return;
    }
  } else if (currentMenu == FIRMWARE_UPDATE_GRID) {
    current_item_list_ptr = otaMenuItems;  // Use the declared variable
  } else {
    return;
  }

  if (!current_item_list_ptr || maxMenuItems == 0) return;  // Use the declared variable

  int item_box_base_width = (128 - (gridCols + 1) * GRID_ITEM_PADDING_X) / gridCols;
  const int grid_content_area_start_y_abs = STATUS_BAR_H + 1;
  const int first_item_row_start_y_abs = grid_content_area_start_y_abs + GRID_ITEM_PADDING_Y;

  u8g2.setFont(u8g2_font_5x7_tf);
  u8g2.setClipWindow(0, grid_content_area_start_y_abs, 127, 63);

  for (int i = 0; i < maxMenuItems; i++) {
    int row = i / gridCols;
    int col = i % gridCols;

    float current_item_effective_scale = gridAnimatingIn ? gridItemScale[i] : 1.0f;
    if (i >= MAX_GRID_ITEMS) current_item_effective_scale = 1.0f;

    int current_item_box_w_scaled = (int)(item_box_base_width * current_item_effective_scale);
    int current_item_box_h_scaled = (int)(GRID_ITEM_H * current_item_effective_scale);

    if (current_item_box_w_scaled < 1 || current_item_box_h_scaled < 1) continue;

    int box_x_abs = GRID_ITEM_PADDING_X + col * (item_box_base_width + GRID_ITEM_PADDING_X) + (item_box_base_width - current_item_box_w_scaled) / 2;
    int box_y_abs_no_scroll = first_item_row_start_y_abs + row * (GRID_ITEM_H + GRID_ITEM_PADDING_Y) + (GRID_ITEM_H - current_item_box_h_scaled) / 2;
    int box_y_on_screen_top = box_y_abs_no_scroll - (int)currentGridScrollOffset_Y_anim;

    if (box_y_on_screen_top + current_item_box_h_scaled <= grid_content_area_start_y_abs || box_y_on_screen_top >= 64) {
      continue;
    }

    bool is_selected_item = (i == menuIndex);
    drawRndBox(box_x_abs, box_y_on_screen_top, current_item_box_w_scaled, current_item_box_h_scaled, 2, is_selected_item);

    if (current_item_effective_scale > 0.7f) {
      u8g2.setDrawColor(is_selected_item ? 0 : 1);
      int text_content_x_start = box_x_abs + 2;
      int text_content_y_baseline = box_y_on_screen_top + (current_item_box_h_scaled - u8g2.getAscent()) / 2 + u8g2.getAscent() - 1;
      int text_available_render_width = current_item_box_w_scaled - 4;

      int text_clip_x1 = max(box_x_abs + 1, 0);
      int text_clip_y1 = max(box_y_on_screen_top + 1, grid_content_area_start_y_abs);
      int text_clip_x2 = min(box_x_abs + current_item_box_w_scaled - 1, 127);
      int text_clip_y2 = min(box_y_on_screen_top + current_item_box_h_scaled - 1, 63);

      if (text_clip_x1 < text_clip_x2 && text_clip_y1 < text_clip_y2) {
        u8g2.setClipWindow(text_clip_x1, text_clip_y1, text_clip_x2, text_clip_y2);

        if (is_selected_item) {
          updateMarquee(text_available_render_width, current_item_list_ptr[i]);  // Use the declared variable
        }

        if (is_selected_item && marqueeActive) {
          u8g2.drawStr(text_content_x_start + (int)marqueeOffset, text_content_y_baseline, marqueeText);
        } else {
          char* display_text_truncated = truncateText(current_item_list_ptr[i], text_available_render_width, u8g2);  // Use the declared variable
          int text_width_pixels = u8g2.getStrWidth(display_text_truncated);
          u8g2.drawStr(text_content_x_start + (text_available_render_width - text_width_pixels) / 2, text_content_y_baseline, display_text_truncated);
        }
        u8g2.setClipWindow(0, grid_content_area_start_y_abs, 127, 63);
      }
      u8g2.setDrawColor(1);
    }
  }
  u8g2.setMaxClipWindow();
}

void drawCustomIcon(int x, int y, int iconType, bool isLarge) {
  // ... (Implementation unchanged) ...
  switch (iconType) {
    case 0:  // Tools Icon
      if (isLarge) {
        u8g2.drawLine(x + 4, y + 11, x + 11, y + 4);
        u8g2.drawBox(x + 2, y + 9, 4, 4);
        u8g2.drawPixel(x + 3, y + 10);
        u8g2.drawPixel(x + 4, y + 9);
        u8g2.drawBox(x + 10, y + 2, 4, 4);
        u8g2.drawPixel(x + 11, y + 5);
        u8g2.drawPixel(x + 12, y + 4);
        u8g2.drawLine(x + 4, y + 4, x + 11, y + 11);
        u8g2.drawBox(x + 2, y + 2, 4, 4);
        u8g2.drawBox(x + 10, y + 10, 4, 3);
      } else {
        u8g2.drawLine(x + 1, y + 5, x + 4, y + 2);
        u8g2.drawPixel(x + 0, y + 5);
        u8g2.drawPixel(x + 1, y + 6);
        u8g2.drawPixel(x + 4, y + 1);
        u8g2.drawPixel(x + 5, y + 2);
        u8g2.drawLine(x + 1, y + 1, x + 5, y + 5);
        u8g2.drawPixel(x + 0, y + 0);
        u8g2.drawPixel(x + 1, y + 0);
        u8g2.drawPixel(x + 0, y + 1);
        u8g2.drawPixel(x + 5, y + 6);
        u8g2.drawPixel(x + 6, y + 5);
        u8g2.drawPixel(x + 6, y + 6);
      }
      break;
    case 1:  // Games Icon
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
    case 2:  // Settings Icon
      if (isLarge) {
        u8g2.drawCircle(x + 8, y + 8, 5);
        u8g2.drawCircle(x + 8, y + 8, 2);
        for (int k = 0; k < 8; k++) {
          float angle = k * PI / 4.0;
          int tx1 = x + 8 + round(cos(angle) * 4);
          int ty1 = y + 8 + round(sin(angle) * 4);
          u8g2.drawTriangle(tx1, ty1,
                            x + 8 + round(cos(angle + 0.15) * 7), y + 8 + round(sin(angle + 0.15) * 7),
                            x + 8 + round(cos(angle - 0.15) * 7), y + 8 + round(sin(angle - 0.15) * 7));
        }
      } else {
        u8g2.drawCircle(x + 4, y + 4, 3);
        u8g2.drawPixel(x + 4, y + 4);
        u8g2.drawPixel(x + 4, y + 0);
        u8g2.drawPixel(x + 4, y + 7);
        u8g2.drawPixel(x + 0, y + 4);
        u8g2.drawPixel(x + 7, y + 4);
        u8g2.drawPixel(x + 1, y + 1);
        u8g2.drawPixel(x + 6, y + 1);
        u8g2.drawPixel(x + 1, y + 6);
        u8g2.drawPixel(x + 6, y + 6);
      }
      break;
    case 3:  // Info Icon
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
    case 4:  // Game Icon: Snake
      if (isLarge) {
        u8g2.drawBox(x + 2, y + 7, 3, 3);
        u8g2.drawBox(x + 5, y + 7, 3, 3);
        u8g2.drawBox(x + 8, y + 7, 3, 3);
        u8g2.drawBox(x + 11, y + 7, 3, 3);
        u8g2.drawBox(x + 11, y + 4, 3, 3);
        u8g2.drawBox(x + 11, y + 1, 3, 3);
        u8g2.drawPixel(x + 3, y + 8);
      } else {
        u8g2.drawHLine(x + 1, y + 4, 4);
        u8g2.drawVLine(x + 4, y + 2, 3);
        u8g2.drawPixel(x + 1, y + 3);
      }
      break;
    case 5:  // Game Icon: Tetris blocks
      if (isLarge) {
        u8g2.drawBox(x + 2, y + 9, 3, 3);
        u8g2.drawBox(x + 2, y + 6, 3, 3);
        u8g2.drawBox(x + 2, y + 3, 3, 3);
        u8g2.drawBox(x + 5, y + 9, 3, 3);
        u8g2.drawBox(x + 9, y + 5, 3, 3);
        u8g2.drawBox(x + 12, y + 5, 3, 3);
        u8g2.drawBox(x + 6, y + 5, 3, 3);
        u8g2.drawBox(x + 9, y + 2, 3, 3);
      } else {
        u8g2.drawBox(x + 1, y + 4, 2, 2);
        u8g2.drawBox(x + 1, y + 2, 2, 2);
        u8g2.drawBox(x + 3, y + 4, 2, 2);
        u8g2.drawBox(x + 5, y + 1, 2, 2);
        u8g2.drawBox(x + 5, y + 3, 2, 2);
      }
      break;
    case 6:  // Game Icon: Pong paddles and ball
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
    case 7:  // Game Icon: Maze
      if (isLarge) {
        u8g2.drawFrame(x + 1, y + 1, 14, 14);
        u8g2.drawVLine(x + 4, y + 1, 10);
        u8g2.drawVLine(x + 10, y + 4, 11);
        u8g2.drawHLine(x + 4, y + 4, 7);
        u8g2.drawHLine(x + 1, y + 10, 7);
        u8g2.drawPixel(x + 13, y + 2);
      } else {
        u8g2.drawFrame(x + 0, y + 0, 8, 8);
        u8g2.drawVLine(x + 2, y + 0, 5);
        u8g2.drawHLine(x + 2, y + 5, 4);
        u8g2.drawVLine(x + 5, y + 2, 6);
      }
      break;
    case 8:  // Navigation Icon: Back Arrow
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
    case 9:  // Network Icon: WiFi Symbol
      if (isLarge) {
        int cx = x + 8;
        int cy = y + 12;
        u8g2.drawDisc(cx, cy, 1.5);
        u8g2.drawCircle(cx, cy, 4, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);
        u8g2.drawCircle(cx, cy, 7, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);
        u8g2.drawCircle(cx, cy, 10, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);
      } else {
        int cx = x + 4;
        int cy = y + 6;
        u8g2.drawDisc(cx, cy, 0);
        u8g2.drawCircle(cx, cy, 2, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);
        u8g2.drawCircle(cx, cy, 4, U8G2_DRAW_UPPER_LEFT | U8G2_DRAW_UPPER_RIGHT);
      }
      break;
    case 10:  // Network Icon: Bluetooth Symbol
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
    case 11:  // Tool Icon: Jamming
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
    case 12:  // Tool Icon: Injection
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
    case 13:  // Settings Icon: Display Options
      if (isLarge) {
        u8g2.drawRFrame(x + 2, y + 2, 12, 9, 1);
        u8g2.drawBox(x + 5, y + 11, 6, 2);
        u8g2.drawVLine(x + 7, y + 11, 2);
        u8g2.drawVLine(x + 8, y + 11, 2);
      } else {
        u8g2.drawFrame(x + 1, y + 1, 6, 4);
        u8g2.drawBox(x + 2, y + 5, 4, 1);
        u8g2.drawVLine(x + 3, y + 5, 2);
      }
      break;
    case 14:  // Settings Icon: Sound Setup
      if (isLarge) {
        u8g2.drawBox(x + 3, y + 4, 5, 8);
        u8g2.drawTriangle(x + 7, y + 4, x + 7, y + 11, x + 10, y + 8);
        u8g2.drawCircle(x + 12, y + 8, 2, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_LOWER_RIGHT);
        u8g2.drawCircle(x + 13, y + 8, 4, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_LOWER_RIGHT);
      } else {
        u8g2.drawBox(x + 1, y + 2, 3, 4);
        u8g2.drawLine(x + 5, y + 3, x + 6, y + 3);
        u8g2.drawLine(x + 5, y + 5, x + 7, y + 5);
      }
      break;
    case 15:  // Settings Icon: System Info
      if (isLarge) {
        u8g2.drawRFrame(x + 3, y + 3, 10, 10, 1);
        u8g2.drawRFrame(x + 5, y + 5, 6, 6, 0);
        for (int k = 0; k < 3; ++k) {
          u8g2.drawHLine(x + 0, y + 4 + (k * 3), 3);
          u8g2.drawHLine(x + 13, y + 4 + (k * 3), 3);
          u8g2.drawVLine(x + 4 + (k * 3), y + 0, 3);
          u8g2.drawVLine(x + 4 + (k * 3), y + 13, 3);
        }
      } else {
        u8g2.drawFrame(x + 1, y + 1, 6, 6);
        u8g2.drawFrame(x + 2, y + 2, 4, 4);
        u8g2.drawPixel(x + 0, y + 3);
        u8g2.drawPixel(x + 7, y + 3);
        u8g2.drawPixel(x + 3, y + 0);
        u8g2.drawPixel(x + 3, y + 7);
      }
      break;
    case 16:  // UI Icon: Refresh / Rescan
      if (isLarge) {
        u8g2.drawCircle(x + 8, y + 8, 6, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_LOWER_RIGHT | U8G2_DRAW_LOWER_LEFT);
        u8g2.drawLine(x + 8, y + 2, x + 5, y + 3);
        u8g2.drawLine(x + 5, y + 3, x + 6, y + 5);
        u8g2.drawLine(x + 5, y + 3, x + 3, y + 5);
      } else {
        u8g2.drawCircle(x + 4, y + 4, 3, U8G2_DRAW_UPPER_RIGHT | U8G2_DRAW_LOWER_RIGHT | U8G2_DRAW_LOWER_LEFT);
        u8g2.drawLine(x + 4, y + 1, x + 4 - 2, y + 1 + 1);
        u8g2.drawLine(x + 4, y + 1, x + 4 - 1, y + 1 + 2);
      }
      break;
    case 17:  // UI Icon: Small Lightning Bolt
      if (isLarge) {
        u8g2.drawLine(x + 5, y + 0, x + 2, y + 6);
        u8g2.drawLine(x + 2, y + 6, x + 7, y + 5);
        u8g2.drawLine(x + 7, y + 5, x + 4, y + 13);
      } else {
        u8g2.drawLine(x + 2, y + 0, x + 0, y + 3);
        u8g2.drawLine(x + 0, y + 3, x + 4, y + 3);
        u8g2.drawLine(x + 4, y + 3, x + 2, y + 6);
      }
      break;
    case 18:  // UI Icon: Vibration Motor
      if (isLarge) {
        u8g2.drawCircle(x + 8, y + 8, 6);
        u8g2.drawBox(x + 8 - 1, y + 8 - 4, 2, 3);
        u8g2.drawDisc(x + 8, y + 8, 1);
      } else {
        u8g2.drawCircle(x + 4, y + 4, 3);
        u8g2.drawPixel(x + 4, y + 1);
        u8g2.drawPixel(x + 4, y + 4);
      }
      break;
    case 19:  // UI Icon: Laser Beam
      if (isLarge) {
        u8g2.drawDisc(x + 8, y + 8, 2);
        for (int k = 0; k < 8; k++) {
          float angle = k * PI / 4.0;
          u8g2.drawLine(x + 8 + round(cos(angle) * 3), y + 8 + round(sin(angle) * 3),
                        x + 8 + round(cos(angle) * 7), y + 8 + round(sin(angle) * 7));
        }
      } else {
        u8g2.drawDisc(x + 4, y + 4, 1);
        u8g2.drawLine(x + 4, y + 0, x + 4, y + 7);
        u8g2.drawLine(x + 0, y + 4, x + 7, y + 4);
        u8g2.drawLine(x + 1, y + 1, x + 6, y + 6);
        u8g2.drawLine(x + 1, y + 6, x + 6, y + 1);
      }
      break;
    case 20:  // UI Icon: Flashlight
      if (isLarge) {
        u8g2.drawTriangle(x + 4, y + 2, x + 12, y + 2, x + 8, y + 0);
        u8g2.drawBox(x + 6, y + 2, 4, 5);
        u8g2.drawLine(x + 2, y + 8, x + 6, y + 14);
        u8g2.drawLine(x + 8, y + 8, x + 8, y + 14);
        u8g2.drawLine(x + 14, y + 8, x + 10, y + 14);
      } else {
        u8g2.drawBox(x + 2, y + 0, 4, 3);
        u8g2.drawLine(x + 0, y + 4, x + 3, y + 7);
        u8g2.drawLine(x + 4, y + 4, x + 4, y + 7);
        u8g2.drawLine(x + 7, y + 4, x + 5, y + 7);
      }
      break;
    case 21:  // UI Icon: Utilities
      if (isLarge) {
        u8g2.drawRFrame(x + 2, y + 5, 12, 8, 1);
        u8g2.drawBox(x + 1, y + 3, 14, 2);
        u8g2.drawRFrame(x + 6, y + 1, 4, 3, 1);
        u8g2.drawPixel(x + 4, y + 8);
        u8g2.drawPixel(x + 11, y + 8);
      } else {
        u8g2.drawFrame(x + 1, y + 3, 6, 4);
        u8g2.drawHLine(x + 0, y + 2, 8);
        u8g2.drawFrame(x + 3, y + 0, 2, 2);
      }
      break;
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
  if (w <= 0 || h <= 0) return;
  if (w <= 2 * r || h <= 2 * r) {
    if (fill) u8g2.drawBox(x, y, w, h);
    else u8g2.drawFrame(x, y, w, h);
    return;
  }
  if (fill) u8g2.drawRBox(x, y, w, h, r);
  else u8g2.drawRFrame(x, y, w, h, r);
}

void drawBatIcon(int x_left, int y_top, uint8_t percentage) {
  int body_width = 7;
  int body_height = 5;
  int tip_width = 1;
  int tip_height = 3;

  u8g2.drawFrame(x_left, y_top, body_width, body_height);

  int tip_y = y_top + (body_height - tip_height) / 2;
  u8g2.drawBox(x_left + body_width, tip_y, tip_width, tip_height);

  int fill_padding_x = 1;
  int fill_padding_y = 1;
  int max_fill_width = body_width - 2 * fill_padding_x;
  int fill_width = (percentage * max_fill_width) / 100;

  if (fill_width > 0) {
    u8g2.drawBox(x_left + fill_padding_x,
                 y_top + fill_padding_y,
                 fill_width,
                 body_height - 2 * fill_padding_y);
  }
}