#include "ui_drawing.h"
#include "battery_monitor.h"
#include "menu_logic.h"
#include "pcf_utils.h"
#include "keyboard_layout.h"
#include "wifi_manager.h"
#include "jamming.h"
#include "ota_manager.h"
#include "firmware_metadata.h"
#include "wifi_attack_tools.h"  // <--- ADD THIS INCLUDE
#include "ui_drawing.h"
#include "icons.h"
#include <U8g2lib.h>
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
  const int list_start_y_abs = STATUS_BAR_H + 1;
  const int list_visible_height = u8g2.getDisplayHeight() - list_start_y_abs;
  const int item_row_h = WIFI_LIST_ITEM_H;
  const int item_padding_x = 2;
  const int content_padding_x = 4;

  u8g2.setClipWindow(0, list_start_y_abs, u8g2.getDisplayWidth(), u8g2.getDisplayHeight());
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setDrawColor(1);

  const char* statusMsg = getWifiStatusMessage();

  // --- Phase 1: Display exclusive status messages if applicable ---
  if (!wifiHardwareEnabled) {
    int msgWidth = u8g2.getStrWidth(statusMsg);  // Should be "Wi-Fi Off"
    int text_y_pos = list_start_y_abs + (list_visible_height - (u8g2.getAscent() - u8g2.getDescent())) / 2 + u8g2.getAscent();
    u8g2.drawStr((u8g2.getDisplayWidth() - msgWidth) / 2, text_y_pos, statusMsg);
    u8g2.setMaxClipWindow();  // Reset clip window before returning
    return;
  } else if (wifiIsScanning) {
    int msgWidth = u8g2.getStrWidth(statusMsg);  // Should be "Scanning..."
    int text_y_pos = list_start_y_abs + (list_visible_height - (u8g2.getAscent() - u8g2.getDescent())) / 2 + u8g2.getAscent();
    u8g2.drawStr((u8g2.getDisplayWidth() - msgWidth) / 2, text_y_pos, statusMsg);
    u8g2.setMaxClipWindow();  // Reset clip window before returning
    return;
  }
  // No early return from here if not off or scanning. Proceed to draw message and/or list.

  // --- Phase 1.5: Display "Scan Failed" or "No Networks" message if applicable, positioned carefully ---
  bool specialMessageDisplayed = false;
  if (wifiHardwareEnabled && !wifiIsScanning && foundWifiNetworksCount == 0 && (strcmp(statusMsg, "Scan Failed") == 0 || strcmp(statusMsg, "No Networks") == 0)) {
    // MaxMenuItems will be 2 here ("Scan Again", "Back"), set by initializeCurrentMenu
    int msgWidth = u8g2.getStrWidth(statusMsg);
    // Position this message. If there are items ("Scan Again", "Back"), they will be drawn by the main loop.
    // Let's position it in the upper part of the list area.
    int text_y_pos_status = list_start_y_abs + item_row_h / 2 + u8g2.getAscent();  // Example: Centered in first potential item slot
    if (list_visible_height < 3 * item_row_h) {                                    // If very little space
      text_y_pos_status = list_start_y_abs + (list_visible_height / 3) + u8g2.getAscent();
    }
    // Ensure message Y doesn't interfere if items are drawn starting at currentWifiListScrollOffset_Y_anim = 0
    if (maxMenuItems > 0) {                                                            // If "Scan Again" / "Back" will be drawn
      int firstItemTopY = list_start_y_abs - (int)currentWifiListScrollOffset_Y_anim;  // Simplified, assuming item 0
      if (text_y_pos_status > firstItemTopY - 5) {                                     // If message might overlap item 0's text
                                                                                       // Push message higher, or adjust item drawing area.
                                                                                       // For simplicity, let's assume items will be drawn below this.
                                                                                       // This Y might need fine-tuning based on how items are drawn below.
      }
    }
    u8g2.drawStr((u8g2.getDisplayWidth() - msgWidth) / 2, text_y_pos_status, statusMsg);
    specialMessageDisplayed = true;
  }

  // --- Phase 2: Draw the list of networks (or "Scan Again" / "Back") ---
  // This part runs if Wi-Fi is ON, NOT scanning.
  // Scroll animation
  if (maxMenuItems > 0) {
    float target_scroll_for_centering = 0;
    if ((maxMenuItems * item_row_h > list_visible_height)) {
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
    wifiListAnim.update();
  } else {
    currentWifiListScrollOffset_Y_anim = 0;
  }

  // Draw list items
  if (maxMenuItems > 0) {
    // If a special message like "Scan Failed" was drawn, and we have few items (e.g. "Scan Again", "Back"),
    // we might want to adjust the starting Y of these items to be below the message.
    // For now, let's assume the currentWifiListScrollOffset_Y_anim and item positioning handles this.
    // If specialMessageDisplayed is true, the items are "Scan Again" and "Back".
    // Their `i` will be 0 and 1, mapping to foundWifiNetworksCount and foundWifiNetworksCount + 1 respectively.

    for (int i = 0; i < maxMenuItems; i++) {
      int item_center_y_in_full_list = (i * item_row_h) + (item_row_h / 2);
      // If specialMessageDisplayed, and drawing "Scan Again" / "Back", start them lower.
      // This offset is ad-hoc, might need better calculation based on message height.
      int y_offset_due_to_message = 0;
      if (specialMessageDisplayed && foundWifiNetworksCount == 0) {
        y_offset_due_to_message = item_row_h + 5;  // Push items down by roughly one row + padding
      }

      item_center_y_in_full_list += y_offset_due_to_message;


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
        drawCustomIcon(current_content_x, icon_y_center_for_draw - 4, action_icon_type, RENDER_SIZE_SMALL);
        current_content_x += 10;
      } else {
        current_content_x += 10;
      }

      int text_x_start = current_content_x;
      int text_available_width = u8g2.getDisplayWidth() - text_x_start - content_padding_x - item_padding_x;
      int text_baseline_y = item_top_on_screen_y + (item_row_h - (u8g2.getAscent() - u8g2.getDescent())) / 2 + u8g2.getAscent();

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
  u8g2.drawStr(2, 7 + STATUS_BAR_H, ssidTitle);  // Y position for "Enter Pass for:"

  char truncatedSsid[22];  // Assuming max 21 chars + null for SSID display here
  strncpy(truncatedSsid, currentSsidToConnect, sizeof(truncatedSsid) - 1);
  truncatedSsid[sizeof(truncatedSsid) - 1] = '\0';
  // Truncate if too long for display
  if (u8g2.getStrWidth(truncatedSsid) > u8g2.getDisplayWidth() - 4) {
    // truncateText uses SBUF, so copy result if needed elsewhere, or use directly if SBUF is safe
    char* tempTrunc = truncateText(currentSsidToConnect, u8g2.getDisplayWidth() - 4, u8g2);
    strncpy(truncatedSsid, tempTrunc, sizeof(truncatedSsid) - 1);
    truncatedSsid[sizeof(truncatedSsid) - 1] = '\0';
  }
  int ssidTextY = 19 + STATUS_BAR_H;  // Y position for the SSID itself
  u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(truncatedSsid)) / 2, ssidTextY, truncatedSsid);

  // Define field properties
  int fieldX = 5;
  // int fieldY = PASSWORD_INPUT_FIELD_Y; // Original
  int fieldY = PASSWORD_INPUT_FIELD_Y + 5;  // MODIFIED: Moved down by 5 pixels
  int fieldW = u8g2.getDisplayWidth() - 10;
  int fieldH = PASSWORD_INPUT_FIELD_H;

  // Ensure fieldY doesn't push it too far down, adjust if necessary
  // For example, if it collides with the instruction text at the bottom:
  int instructionTextY_approx = u8g2.getDisplayHeight() - 2 - u8g2.getAscent();  // Approximate top of instruction text
  if (fieldY + fieldH > instructionTextY_approx - 2) {                           // If field bottom is too close to instruction top
    fieldY = instructionTextY_approx - fieldH - 2;                               // Adjust upwards slightly
    if (fieldY < ssidTextY + u8g2.getAscent() + 2) {                             // Ensure it's still below SSID text
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

  char displaySegment[maxPassDisplayLengthChars + 2];  // +1 for potential char, +1 for null
  strncpy(displaySegment, maskedPassword + passDisplayStartIdx, maxPassDisplayLengthChars);
  displaySegment[maxPassDisplayLengthChars] = '\0';  // Ensure null termination
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
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setDrawColor(1);

  const char* resultMsgFull = getWifiStatusMessage();
  int iconY = 10;
  int textYBase = 32 + 7;  // Shifted down by 7 pixels from original 32

  WifiConnectionStatus lastStatus = getCurrentWifiConnectionStatus();

  if (lastStatus == WIFI_CONNECTED_SUCCESS) {
    drawCustomIcon(u8g2.getDisplayWidth() / 2 - 8, iconY, 3);  // Success icon

    String connectedPrefix = "Connected: ";
    String ssidName = "";

    // Extract SSID name from the full message "Connected: ActualSSID"
    if (String(resultMsgFull).startsWith(connectedPrefix)) {
      ssidName = String(resultMsgFull).substring(connectedPrefix.length());
    } else {
      // Fallback: display full message if parsing fails (should ideally not happen for success)
      int msgWidth = u8g2.getStrWidth(resultMsgFull);
      if (msgWidth > u8g2.getDisplayWidth() - 4) {
        char* truncatedMsg = truncateText(resultMsgFull, u8g2.getDisplayWidth() - 4, u8g2);
        u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(truncatedMsg)) / 2, textYBase, truncatedMsg);
      } else {
        u8g2.drawStr((u8g2.getDisplayWidth() - msgWidth) / 2, textYBase, resultMsgFull);
      }
      return;
    }

    int prefixWidth = u8g2.getStrWidth(connectedPrefix.c_str());
    int ssidNameWidth = u8g2.getStrWidth(ssidName.c_str());
    int availableWidth = u8g2.getDisplayWidth() - 4;  // Usable width for text

    // Check if the prefix plus a space plus the SSID name fits on one line
    if (prefixWidth + 1 /* for space */ + ssidNameWidth > availableWidth && prefixWidth < availableWidth) {
      // Word wrap: Display "Connected:" on the first line
      u8g2.drawStr((u8g2.getDisplayWidth() - prefixWidth) / 2, textYBase, connectedPrefix.c_str());

      // Display SSID on the second line
      int ssidTextY = textYBase + 10;        // Approx line height for 6x10 font
      if (ssidNameWidth > availableWidth) {  // If SSID itself is too long for one line
        char* truncatedSsid = truncateText(ssidName.c_str(), availableWidth, u8g2);
        u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(truncatedSsid)) / 2, ssidTextY, truncatedSsid);
      } else {
        u8g2.drawStr((u8g2.getDisplayWidth() - ssidNameWidth) / 2, ssidTextY, ssidName.c_str());
      }
    } else {  // Fits on one line, or prefix itself is too long (truncate the whole original message)
      int fullMsgWidth = u8g2.getStrWidth(resultMsgFull);
      if (fullMsgWidth > availableWidth) {
        char* truncatedFullMsg = truncateText(resultMsgFull, availableWidth, u8g2);
        u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(truncatedFullMsg)) / 2, textYBase, truncatedFullMsg);
      } else {
        u8g2.drawStr((u8g2.getDisplayWidth() - fullMsgWidth) / 2, textYBase, resultMsgFull);
      }
    }

  } else {                                                      // Error case
    drawCustomIcon(u8g2.getDisplayWidth() / 2 - 8, iconY, 11);  // Error icon
    int msgWidth = u8g2.getStrWidth(resultMsgFull);
    // For error messages, just display as before but shifted down
    if (msgWidth > u8g2.getDisplayWidth() - 4) {
      char* truncatedMsg = truncateText(resultMsgFull, u8g2.getDisplayWidth() - 4, u8g2);
      u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(truncatedMsg)) / 2, textYBase, truncatedMsg);
    } else {
      u8g2.drawStr((u8g2.getDisplayWidth() - msgWidth) / 2, textYBase, resultMsgFull);
    }
  }
}

void drawPromptOverlay() {  // WAS drawWifiDisconnectOverlay
  // --- Animation Update ---
  if (promptOverlayAnimatingIn) {  // Use new variable names
    float diff = promptOverlayTargetScale - promptOverlayCurrentScale;
    if (abs(diff) > 0.01f) {
      promptOverlayCurrentScale += diff * (GRID_ANIM_SPEED * 1.5f) * 0.016f;
      if ((diff > 0 && promptOverlayCurrentScale > promptOverlayTargetScale) || (diff < 0 && promptOverlayCurrentScale < promptOverlayTargetScale)) {
        promptOverlayCurrentScale = promptOverlayTargetScale;
      }
    } else {
      promptOverlayCurrentScale = promptOverlayTargetScale;
      promptOverlayAnimatingIn = false;
    }
  }

  if (promptOverlayCurrentScale <= 0.05f && !promptOverlayAnimatingIn) return;

  int baseOverlayWidth = 110;  // Slightly wider for more generic text
  int baseOverlayHeight = 55;  // Adjusted height

  int animatedOverlayWidth = (int)(baseOverlayWidth * promptOverlayCurrentScale);
  int animatedOverlayHeight = (int)(baseOverlayHeight * promptOverlayCurrentScale);
  if (animatedOverlayWidth < 1 || animatedOverlayHeight < 1) return;

  int drawX = (u8g2.getDisplayWidth() - baseOverlayWidth) / 2 + (baseOverlayWidth - animatedOverlayWidth) / 2;
  int drawY = (u8g2.getDisplayHeight() - baseOverlayHeight) / 2 + (baseOverlayHeight - animatedOverlayHeight) / 2;
  drawY = max(drawY, STATUS_BAR_H + 3);


  int padding = 4;
  u8g2.setFont(u8g2_font_6x10_tf);
  int fontAscent = u8g2.getAscent();
  int textLineHeight = fontAscent + 2;  // Slightly tighter

  u8g2.setDrawColor(1);
  u8g2.drawRBox(drawX, drawY, animatedOverlayWidth, animatedOverlayHeight, 3);
  u8g2.setDrawColor(0);
  u8g2.drawRFrame(drawX, drawY, animatedOverlayWidth, animatedOverlayHeight, 3);

  if (promptOverlayCurrentScale > 0.9f) {
    u8g2.setDrawColor(0);

    // Title
    int titleX = drawX + (animatedOverlayWidth - u8g2.getStrWidth(promptOverlayTitle)) / 2;
    int titleY = drawY + padding + fontAscent;
    u8g2.drawStr(titleX, titleY, promptOverlayTitle);

    // Message (potentially two lines)
    String msgStr = promptOverlayMessage;
    String msgLine1 = msgStr;
    String msgLine2 = "";
    int msgMaxPixelWidth = animatedOverlayWidth - 2 * padding - 4;

    if (u8g2.getStrWidth(msgStr.c_str()) > msgMaxPixelWidth) {
      // Attempt to split message smartly
      int breakPt = -1;
      for (int k = msgStr.length() / 2; k < msgStr.length(); ++k) {
        if (msgStr.charAt(k) == ' ') {
          breakPt = k;
          break;
        }
      }
      if (breakPt == -1)
        for (int k = msgStr.length() / 2 - 1; k >= 0; --k) {
          if (msgStr.charAt(k) == ' ') {
            breakPt = k;
            break;
          }
        }

      if (breakPt != -1) {
        msgLine1 = msgStr.substring(0, breakPt);
        msgLine2 = msgStr.substring(breakPt + 1);
      } else {  // Hard split if no space
        int approxChars = msgMaxPixelWidth / u8g2.getMaxCharWidth();
        if (approxChars > 0 && msgStr.length() > approxChars) {
          msgLine1 = msgStr.substring(0, approxChars);
          msgLine2 = msgStr.substring(approxChars);
        }
      }
    }
    int msgY1 = titleY + textLineHeight;
    u8g2.drawStr(drawX + (animatedOverlayWidth - u8g2.getStrWidth(msgLine1.c_str())) / 2, msgY1, msgLine1.c_str());
    if (msgLine2.length() > 0) {
      int msgY2 = msgY1 + textLineHeight;
      u8g2.drawStr(drawX + (animatedOverlayWidth - u8g2.getStrWidth(msgLine2.c_str())) / 2, msgY2, msgLine2.c_str());
    }


    // Options
    int optionTextY = drawY + animatedOverlayHeight - padding - fontAscent - 1;  // Baseline for text
    int optionBoxY = optionTextY - fontAscent - 2;

    int opt0Width = u8g2.getStrWidth(promptOverlayOption0Text);
    int opt1Width = u8g2.getStrWidth(promptOverlayOption1Text);
    int optionBoxWidth = max(opt0Width, opt1Width) + 12;  // Ensure enough padding
    int optionBoxHeight = fontAscent + 4;

    int totalOptionsWidth = 2 * optionBoxWidth + 10;  // 10px spacing
    int optionsStartX = drawX + (animatedOverlayWidth - totalOptionsWidth) / 2;

    int opt0BoxX = optionsStartX;
    int opt1BoxX = optionsStartX + optionBoxWidth + 10;

    // Draw Option 0 (Cancel/No)
    if (promptOverlaySelection == 0) {
      u8g2.setDrawColor(0);
      u8g2.drawRBox(opt0BoxX, optionBoxY, optionBoxWidth, optionBoxHeight, 2);
      u8g2.setDrawColor(1);
    } else {
      u8g2.setDrawColor(0);
      u8g2.drawRFrame(opt0BoxX, optionBoxY, optionBoxWidth, optionBoxHeight, 2);
    }
    u8g2.drawStr(opt0BoxX + (optionBoxWidth - opt0Width) / 2, optionTextY, promptOverlayOption0Text);

    // Draw Option 1 (Confirm/Yes)
    u8g2.setDrawColor(0);  // Reset for frame or fill
    if (promptOverlaySelection == 1) {
      u8g2.drawRBox(opt1BoxX, optionBoxY, optionBoxWidth, optionBoxHeight, 2);
      u8g2.setDrawColor(1);
    } else {
      u8g2.drawRFrame(opt1BoxX, optionBoxY, optionBoxWidth, optionBoxHeight, 2);
    }
    u8g2.drawStr(opt1BoxX + (optionBoxWidth - opt1Width) / 2, optionTextY, promptOverlayOption1Text);
  }
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

void drawOtaStatusScreen() {
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setDrawColor(1);

  const char* title = "Firmware Update";
  if (currentMenu == OTA_WEB_ACTIVE) title = "Web Update";
  else if (currentMenu == OTA_SD_STATUS) title = "SD Card Update";
  else if (currentMenu == OTA_BASIC_ACTIVE) title = "Basic OTA (IDE)";

  int currentY = STATUS_BAR_H + 6;
  if (!(currentMenu == WIFI_PASSWORD_INPUT || currentMenu == WIFI_CONNECTING || currentMenu == WIFI_CONNECTION_INFO || currentMenu == JAMMING_ACTIVE_SCREEN || currentMenu == OTA_WEB_ACTIVE || currentMenu == OTA_SD_STATUS || currentMenu == OTA_BASIC_ACTIVE)) {
    // This implies status bar IS drawn, so currentY is relative to it
  } else {
    currentY = 8;  // Status bar NOT drawn, start title lower
  }

  int titleWidth = u8g2.getStrWidth(title);
  u8g2.drawStr((u8g2.getDisplayWidth() - titleWidth) / 2, currentY, title);
  currentY += 12;

  // --- Display IP and Password (Unified logic for Web and Basic OTA) ---
  if ((currentMenu == OTA_WEB_ACTIVE || currentMenu == OTA_BASIC_ACTIVE) && strlen(otaDisplayIpAddress) > 0) {
    String fullInfo = otaDisplayIpAddress;
    String ipLine = "";
    String passwordLine = "";
    int pColonPos = fullInfo.indexOf(" P: ");

    if (pColonPos != -1) {
      ipLine = fullInfo.substring(0, pColonPos);
      passwordLine = fullInfo.substring(pColonPos);
    } else {
      ipLine = fullInfo;
    }

    int ipLineWidth = u8g2.getStrWidth(ipLine.c_str());
    if (ipLineWidth > u8g2.getDisplayWidth() - 4) {
      updateMarquee(u8g2.getDisplayWidth() - 4, ipLine.c_str());
      if (marqueeActive && strcmp(marqueeText, ipLine.c_str()) == 0) {
        u8g2.drawStr(2 + (int)marqueeOffset, currentY, marqueeText);
      } else {
        char* truncatedIp = truncateText(ipLine.c_str(), u8g2.getDisplayWidth() - 4, u8g2);
        u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(truncatedIp)) / 2, currentY, truncatedIp);
      }
    } else {
      marqueeActive = false;
      u8g2.drawStr((u8g2.getDisplayWidth() - ipLineWidth) / 2, currentY, ipLine.c_str());
    }
    currentY += 14;

    if (passwordLine.length() > 0) {
      int passwordLineWidth = u8g2.getStrWidth(passwordLine.c_str());
      int availablePasswordWidth = u8g2.getDisplayWidth() - 4;

      if (passwordLineWidth > availablePasswordWidth) {
        updateMarquee(availablePasswordWidth, passwordLine.c_str());
        if (marqueeActive && strcmp(marqueeText, passwordLine.c_str()) == 0) {
          u8g2.drawStr(4 + (int)marqueeOffset, currentY, marqueeText);
        } else {
          char* truncatedPass = truncateText(passwordLine.c_str(), availablePasswordWidth, u8g2);
          u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(truncatedPass)) / 2, currentY, truncatedPass);
        }
      } else {
        if (!marqueeActive || strcmp(marqueeText, passwordLine.c_str()) != 0) {
          marqueeActive = false;
        }
        u8g2.drawStr((u8g2.getDisplayWidth() - passwordLineWidth) / 2, currentY, passwordLine.c_str());
      }
      currentY += 2;
    }
    currentY += 10;
  }

  // --- Display otaStatusMessage (handles multi-line and truncation) ---
  if (otaStatusMessage.length() > 0) {
    String line1 = "", line2 = "";
    int maxCharsPerLine = (u8g2.getDisplayWidth() - 8) / u8g2.getStrWidth("W");  // Approximate

    if (otaStatusMessage.length() > maxCharsPerLine && u8g2.getStrWidth(otaStatusMessage.c_str()) > u8g2.getDisplayWidth() - 8) {
      int breakPoint = -1;
      int estimatedPixelBreak = u8g2.getDisplayWidth() - 8;
      int currentPixelWidth = 0;
      int lastGoodBreak = -1;

      for (int k = 0; k < otaStatusMessage.length(); ++k) {
        char tempStr[2] = { otaStatusMessage.charAt(k), '\0' };
        currentPixelWidth += u8g2.getStrWidth(tempStr);
        if (currentPixelWidth > estimatedPixelBreak) {
          if (lastGoodBreak != -1) {
            breakPoint = lastGoodBreak;
          } else {
            breakPoint = k - 1;
            if (breakPoint < 0) breakPoint = 0;
          }
          break;
        }
        if (otaStatusMessage.charAt(k) == ' ' || otaStatusMessage.charAt(k) == '-' || otaStatusMessage.charAt(k) == ':') {
          lastGoodBreak = k;
        }
      }
      if (breakPoint == -1 && currentPixelWidth <= estimatedPixelBreak) {
        line1 = otaStatusMessage;
      } else if (breakPoint != -1) {
        line1 = otaStatusMessage.substring(0, breakPoint + 1);
        line2 = otaStatusMessage.substring(breakPoint + 1);
        line2.trim();
      } else {  // Fallback if no good break point found
        line1 = otaStatusMessage.substring(0, maxCharsPerLine);
        line2 = otaStatusMessage.substring(maxCharsPerLine);
      }
    } else {
      line1 = otaStatusMessage;
    }

    int status1Width = u8g2.getStrWidth(line1.c_str());
    if (status1Width > u8g2.getDisplayWidth() - 4) {
      updateMarquee(u8g2.getDisplayWidth() - 4, line1.c_str());
      if (marqueeActive && strcmp(marqueeText, line1.c_str()) == 0) {
        u8g2.drawStr(2 + (int)marqueeOffset, currentY, marqueeText);
      } else {
        char* truncatedMsg1 = truncateText(line1.c_str(), u8g2.getDisplayWidth() - 4, u8g2);
        u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(truncatedMsg1)) / 2, currentY, truncatedMsg1);
      }
    } else {
      if (marqueeActive && strcmp(marqueeText, line1.c_str()) != 0) marqueeActive = false;
      u8g2.drawStr((u8g2.getDisplayWidth() - status1Width) / 2, currentY, line1.c_str());
    }

    if (line2.length() > 0) {
      currentY += 10;
      int status2Width = u8g2.getStrWidth(line2.c_str());
      if (status2Width > u8g2.getDisplayWidth() - 4) {
        updateMarquee(u8g2.getDisplayWidth() - 4, line2.c_str());
        if (marqueeActive && strcmp(marqueeText, line2.c_str()) == 0) {
          u8g2.drawStr(2 + (int)marqueeOffset, currentY, marqueeText);
        } else {
          char* truncatedMsg2 = truncateText(line2.c_str(), u8g2.getDisplayWidth() - 4, u8g2);
          u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(truncatedMsg2)) / 2, currentY, truncatedMsg2);
        }
      } else {
        if (marqueeActive && strcmp(marqueeText, line2.c_str()) != 0) marqueeActive = false;
        u8g2.drawStr((u8g2.getDisplayWidth() - status2Width) / 2, currentY, line2.c_str());
      }
    }
    // currentY += 12; // Adjusted to provide space for potential error message below
  }

  // --- Conditional Error Message Display (where progress bar used to be) ---
  int bottomElementY = u8g2.getDisplayHeight() - 15;
  int barH_placeholder = 7;  // Height of the area previously occupied by the bar

  // Adjust currentY if it's too close to the bottomElementY, to prevent overlap
  if (currentY > bottomElementY - barH_placeholder - 2) {
    currentY = bottomElementY - barH_placeholder - 2;
  }


  if (otaProgress == -1) {  // Error state
    const char* errorText = "Error Occurred";
    int errorTextWidth = u8g2.getStrWidth(errorText);
    // Draw the error message in the area where the progress bar would have been
    u8g2.drawStr((u8g2.getDisplayWidth() - errorTextWidth) / 2, bottomElementY + (barH_placeholder - (u8g2.getAscent() - u8g2.getDescent())) / 2 + u8g2.getAscent(), errorText);
  }
  // The visual progress bar (frame and fill) is now removed.
  // The otaStatusMessage above will still show textual progress like "Progress: 50%" if it's part of the message.
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
      drawCustomIcon(current_content_x, icon_y_center_for_draw - 4, action_icon_type);  // Small icon
      current_content_x += 10;                                                          // Space for icon
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

void drawBeaconSpamActiveScreen() {
  u8g2.setFont(u8g2_font_7x13B_tr);
  u8g2.setDrawColor(1);

  // Title
  const char* title = "Beacon Spam Active";
  u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(title)) / 2, 24, title);

  // Show which mode is active
  u8g2.setFont(u8g2_font_6x10_tf);
  BeaconSsidMode mode = get_beacon_ssid_mode();
  const char* mode_text = (mode == BeaconSsidMode::FROM_FILE) ? "Mode: SD Card" : "Mode: Random";
  u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(mode_text)) / 2, 42, mode_text);

  // Instruction Text
  const char* instruction = "Press BACK to Stop";
  u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(instruction)) / 2, 58, instruction);
}

void drawRickRollActiveScreen() {
  u8g2.setFont(u8g2_font_7x13B_tr);
  u8g2.setDrawColor(1);

  // Title
  const char* title = "Rick Roll Active";
  u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(title)) / 2, 32, title);

  // Instruction Text
  u8g2.setFont(u8g2_font_6x10_tf);
  const char* instruction = "Press BACK to Stop";
  u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(instruction)) / 2, 58, instruction);
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
  if (currentMenu == WIFI_PASSWORD_INPUT || currentMenu == WIFI_CONNECTING || currentMenu == WIFI_CONNECTION_INFO || currentMenu == JAMMING_ACTIVE_SCREEN || currentMenu == OTA_WEB_ACTIVE || currentMenu == OTA_SD_STATUS || currentMenu == OTA_BASIC_ACTIVE || currentMenu == WIFI_BEACON_SPAM_ACTIVE_SCREEN) {  // <-- ADDED
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
  } else if (currentMenu == WIFI_BEACON_SPAM_ACTIVE_SCREEN) {
    drawBeaconSpamActiveScreen();
  } else if (currentMenu == WIFI_RICK_ROLL_ACTIVE_SCREEN) {
    drawRickRollActiveScreen();
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
  if (showPromptOverlay) {
    drawPromptOverlay();
  }

  u8g2.sendBuffer();
}

void drawPassiveDisplay() {
  if (currentMenu == WIFI_PASSWORD_INPUT) {
    drawKeyboardOnSmallDisplay();
  } else {
    u8g2_small.clearBuffer();
    u8g2_small.setDrawColor(1);

    // Battery Info Section
    float v_bat = currentBatteryVoltage;
    uint8_t s_bat = batPerc(v_bat);
    char batInfoStr[15];
    snprintf(batInfoStr, sizeof(batInfoStr), "%.2fV %s%d%%", v_bat, isCharging ? "+" : "", s_bat);
    u8g2_small.setFont(u8g2_font_5x7_tf);
    int bat_info_y = u8g2_small.getAscent() + 2;
    u8g2_small.drawStr((u8g2_small.getDisplayWidth() - u8g2_small.getStrWidth(batInfoStr)) / 2, bat_info_y, batInfoStr);

    const char* modeText = "---";  // Default
    if (isJammingOperationActive) {
      switch (activeJammingType) {
        case JAM_BLE: modeText = "BLE Jamming"; break;
        case JAM_BT: modeText = "BT Jamming"; break;
        case JAM_NRF_CHANNELS: modeText = "NRF Jamming"; break;
        case JAM_RF_NOISE_FLOOD: modeText = "RF Flooding"; break;
        default: modeText = "Jammer Active"; break;
      }
    } else if (showPromptOverlay) {
      // Check specific prompt titles for more context
      if (strcmp(promptOverlayTitle, "Disconnect WiFi") == 0) {
        modeText = "Disconnecting...";
      } else if (strcmp(promptOverlayTitle, "OTA Wi-Fi") == 0) {
        modeText = "Wi-Fi Needed";  // Or similar concise message for this prompt
      } else {
        // Generic message for other prompts, or use promptOverlayTitle if short enough
        if (strlen(promptOverlayTitle) < 18) {  // Arbitrary length check
          modeText = promptOverlayTitle;
        } else {
          modeText = "Confirm Action";
        }
      }
    } else if (currentMenu == FLASHLIGHT_MODE) {
      modeText = "Torch";
    } else if (currentMenu == MAIN_MENU && menuIndex < getMainMenuItemsCount()) {
      modeText = mainMenuItems[menuIndex];
    } else if (currentMenu == UTILITIES_MENU && menuIndex < getUtilitiesMenuItemsCount()) {
      modeText = utilitiesMenuItems[menuIndex];
    } else if (currentMenu == GAMES_MENU && menuIndex < getGamesMenuItemsCount() && strcmp(gamesMenuItems[menuIndex], "Back") != 0) {
      modeText = gamesMenuItems[menuIndex];
    } else if (currentMenu == TOOLS_MENU && menuIndex < getToolsMenuItemsCount() && strcmp(toolsMenuItems[menuIndex], "Back") != 0) {
      modeText = toolsMenuItems[menuIndex];
    } else if (currentMenu == SETTINGS_MENU && menuIndex < getSettingsMenuItemsCount() && strcmp(settingsMenuItems[menuIndex], "Back") != 0) {
      modeText = settingsMenuItems[menuIndex];
    } else if (currentMenu == TOOL_CATEGORY_GRID) {
      if (toolsCategoryIndex >= 0 && toolsCategoryIndex < getToolsMenuItemsCount()) {
        if (strcmp(toolsMenuItems[toolsCategoryIndex], "Jamming") == 0) {
          if (menuIndex < getJammingToolItemsCount() && strcmp(jammingToolItems[menuIndex], "Back") != 0) {
            modeText = jammingToolItems[menuIndex];
          } else {
            modeText = "Jamming Tools";  // Or toolsMenuItems[toolsCategoryIndex]
          }
        } else if (menuIndex < maxMenuItems - 1) {  // Check if not "Back" item in other grids
          const char** currentGridItems = nullptr;
          switch (toolsCategoryIndex) {
            case 0: currentGridItems = injectionToolItems; break;
            case 1: currentGridItems = wifiAttackToolItems; break;
            case 2: currentGridItems = bleAttackToolItems; break;
            case 3: currentGridItems = nrfReconToolItems; break;
          }
          if (currentGridItems && menuIndex < (currentGridItems == injectionToolItems ? getInjectionToolItemsCount() : currentGridItems == wifiAttackToolItems ? getWifiAttackToolItemsCount()
                                                                                                                     : currentGridItems == bleAttackToolItems  ? getBleAttackToolItemsCount()
                                                                                                                                                               : getNrfReconToolItemsCount())
                                                - 1) {
            modeText = currentGridItems[menuIndex];
          } else {
            modeText = toolsMenuItems[toolsCategoryIndex];  // Category name
          }
        } else {                                          // It's the "Back" item in a tool category grid
          modeText = toolsMenuItems[toolsCategoryIndex];  // Show category name
        }
      }
    } else if (currentMenu == WIFI_SETUP_MENU) {
      if (!wifiHardwareEnabled) modeText = "Wi-Fi Off";
      else if (wifiIsScanning) modeText = "Scanning WiFi";
      else if (foundWifiNetworksCount == 0) modeText = "No Networks";
      else if (wifiMenuIndex < foundWifiNetworksCount) {  // A network is selected
        modeText = scannedNetworks[wifiMenuIndex].ssid;
      } else if (wifiMenuIndex == foundWifiNetworksCount) {
        modeText = "Scan Again";
      } else {  // Back
        modeText = "Wi-Fi Options";
      }
    } else if (currentMenu == WIFI_CONNECTING) {
      modeText = "Connecting...";
    } else if (currentMenu == WIFI_CONNECTION_INFO) {
      modeText = getWifiStatusMessage();  // This should be "Connected: SSID" or error
    } else if (currentMenu == FIRMWARE_UPDATE_GRID) {
      if (menuIndex < getOtaMenuItemsCount()) modeText = otaMenuItems[menuIndex];
      else modeText = "FW Update";
    } else if (currentMenu == FIRMWARE_SD_LIST_MENU) {
      if (availableSdFirmwareCount > 0 && wifiMenuIndex < availableSdFirmwareCount) {
        modeText = availableSdFirmwares[wifiMenuIndex].version;
      } else if (wifiMenuIndex == availableSdFirmwareCount) {  // "Back" item
        modeText = "SD FW List";
      } else {  // No firmwares, only "Back"
        modeText = "SD FW List";
      }
    } else if (currentMenu == OTA_WEB_ACTIVE) {
      modeText = "Web OTA Active";
    } else if (currentMenu == OTA_SD_STATUS) {
      modeText = "SD OTA Status";
    } else if (currentMenu == OTA_BASIC_ACTIVE) {
      modeText = "Basic OTA";
    }


    u8g2_small.setFont(u8g2_font_6x12_tr);
    char truncatedModeText[22];  // Max ~21 chars + null for 128px wide with 6px font
    strncpy(truncatedModeText, modeText, sizeof(truncatedModeText) - 1);
    truncatedModeText[sizeof(truncatedModeText) - 1] = '\0';

    if (u8g2_small.getStrWidth(truncatedModeText) > u8g2_small.getDisplayWidth() - 4) {
      // Use the global SBUF for truncateText as it's designed for
      truncateText(modeText, u8g2_small.getDisplayWidth() - 4, u8g2_small);
      strncpy(truncatedModeText, SBUF, sizeof(truncatedModeText) - 1);
      truncatedModeText[sizeof(truncatedModeText) - 1] = '\0';
    }
    int mode_text_y_baseline = u8g2_small.getDisplayHeight() - 2;
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
  } else if (currentMenu == FIRMWARE_UPDATE_GRID) {
    titleText = "Firmware Update";
  } else if (currentMenu == FIRMWARE_SD_LIST_MENU) {
    titleText = "Update from SD";
  } else if (currentMenu != MAIN_MENU && mainMenuSavedIndex >= 0 && mainMenuSavedIndex < getMainMenuItemsCount()) {
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
    drawCustomIcon(current_x_origin_for_battery, charge_icon_y_top, 17, RENDER_SIZE_SMALL);
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

    drawCustomIcon(icon_x_pos, icon_y_pos, iconType);

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
      drawCustomIcon(icon_x_pos, icon_y_pos, icon_type_for_item);

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

void drawCustomIcon(int x_top_left, int y_top_left, int iconType, IconRenderSize size /*= RENDER_SIZE_DEFAULT*/) {
  const unsigned char* xbm_data = nullptr;
  int w = 0, h = 0;
  bool icon_resolved = false;  // Flag to track if we've successfully resolved an icon to draw

  // --- Phase 1: Try to resolve a small icon if requested and available ---
  if (size == RENDER_SIZE_SMALL) {
    switch (static_cast<IconType>(iconType)) {  // Cast to IconType enum for switch
      case ICON_UI_CHARGING_BOLT:
        xbm_data = icon_ui_charging_bolt_small_bits;  // Assumes this is defined in icons.cpp
        w = SMALL_ICON_WIDTH;
        h = SMALL_ICON_HEIGHT;
        icon_resolved = true;
        break;
      // Add other cases here for icons that have specific _small_bits definitions
      case ICON_UI_REFRESH:
        xbm_data = icon_ui_refresh_small_bits;
        w = SMALL_ICON_WIDTH;
        h = SMALL_ICON_HEIGHT;
        icon_resolved = true;
        break;
      case ICON_NAV_BACK:
        xbm_data = icon_nav_back_small_bits;
        w = SMALL_ICON_WIDTH;
        h = SMALL_ICON_HEIGHT;
        icon_resolved = true;
        break;
      default:
        // No specific small version for this iconType, will fall through to large/default
        break;
    }
  }

  // --- Phase 2: Resolve to large/default icon if small wasn't resolved or wasn't requested ---
  // This also handles RENDER_SIZE_DEFAULT: if a small one was specifically defined for RENDER_SIZE_SMALL above
  // and RENDER_SIZE_DEFAULT is passed, it would have already been picked. Otherwise, default is large.
  // It also handles RENDER_SIZE_LARGE explicitly.
  if (!icon_resolved) {
    w = LARGE_ICON_WIDTH;
    h = LARGE_ICON_HEIGHT;
    icon_resolved = true;  // Assume a large version exists for all valid icon types

    switch (static_cast<IconType>(iconType)) {
      case ICON_TOOLS: xbm_data = icon_tools_large_bits; break;
      case ICON_GAMES: xbm_data = icon_games_large_bits; break;
      case ICON_SETTINGS: xbm_data = icon_settings_large_bits; break;
      case ICON_INFO: xbm_data = icon_info_large_bits; break;
      case ICON_GAME_SNAKE: xbm_data = icon_game_snake_large_bits; break;
      case ICON_GAME_TETRIS: xbm_data = icon_game_tetris_large_bits; break;
      case ICON_GAME_PONG: xbm_data = icon_game_pong_large_bits; break;
      case ICON_GAME_MAZE: xbm_data = icon_game_maze_large_bits; break;
      case ICON_NAV_BACK: xbm_data = icon_nav_back_large_bits; break;
      case ICON_NET_WIFI: xbm_data = icon_net_wifi_large_bits; break;
      case ICON_NET_BLUETOOTH: xbm_data = icon_net_bluetooth_large_bits; break;
      case ICON_TOOL_JAMMING: xbm_data = icon_tool_jamming_large_bits; break;
      case ICON_TOOL_INJECTION: xbm_data = icon_tool_injection_large_bits; break;
      case ICON_SETTING_DISPLAY: xbm_data = icon_setting_display_large_bits; break;
      case ICON_SETTING_SOUND: xbm_data = icon_setting_sound_large_bits; break;
      case ICON_SETTING_SYSTEM: xbm_data = icon_setting_system_large_bits; break;
      case ICON_UI_REFRESH: xbm_data = icon_ui_refresh_large_bits; break;
      case ICON_UI_CHARGING_BOLT:  // This is the large version, used if small wasn't explicitly chosen or doesn't exist
        xbm_data = icon_ui_charging_bolt_large_bits;
        break;
      case ICON_UI_VIBRATION: xbm_data = icon_ui_vibration_large_bits; break;
      case ICON_UI_LASER: xbm_data = icon_ui_laser_large_bits; break;
      case ICON_UI_FLASHLIGHT: xbm_data = icon_ui_flashlight_large_bits; break;
      case ICON_UTILITIES_CATEGORY: xbm_data = icon_utilities_category_large_bits; break;
      default:
        icon_resolved = false;  // Invalid iconType, or iconType out of range
        break;
    }
  }

  // --- Phase 3: Draw the resolved icon or a placeholder ---
  if (icon_resolved && xbm_data != nullptr && w > 0 && h > 0) {
    u8g2.drawXBM(x_top_left, y_top_left, w, h, xbm_data);
  } else {
    // Fallback placeholder for unknown icon type or if XBM data is somehow null
    // Determine placeholder size based on what was intended
    int placeholder_w, placeholder_h;
    if (size == RENDER_SIZE_SMALL) {
      placeholder_w = SMALL_ICON_WIDTH;
      placeholder_h = SMALL_ICON_HEIGHT;
    } else {  // RENDER_SIZE_LARGE or RENDER_SIZE_DEFAULT
      placeholder_w = LARGE_ICON_WIDTH;
      placeholder_h = LARGE_ICON_HEIGHT;
    }
    // If w and h were resolved but xbm_data was null, use those dimensions for placeholder
    if (w > 0) placeholder_w = w;
    if (h > 0) placeholder_h = h;


    u8g2.setDrawColor(1);  // Ensure placeholder is drawn in foreground color
    u8g2.drawFrame(x_top_left, y_top_left, placeholder_w, placeholder_h);
    u8g2.drawLine(x_top_left, y_top_left, x_top_left + placeholder_w - 1, y_top_left + placeholder_h - 1);
    u8g2.drawLine(x_top_left + placeholder_w - 1, y_top_left, x_top_left, y_top_left + placeholder_h - 1);
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