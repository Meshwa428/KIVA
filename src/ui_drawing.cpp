#include "ui_drawing.h"
#include "battery_monitor.h"
#include "menu_logic.h"
#include "pcf_utils.h"
#include "keyboard_layout.h" // For keyboard structures and definitions
#include "wifi_manager.h"    // For getWifiStatusMessage()
#include <Arduino.h>

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
    const int item_row_h = WIFI_LIST_ITEM_H;
    const int item_padding_x = 2; 
    const int content_padding_x = 4; // Padding for content within each item row

    // Set a clipping window for the list area to prevent drawing over status bar or below screen
    u8g2.setClipWindow(0, list_start_y_abs, u8g2.getDisplayWidth(), u8g2.getDisplayHeight());
    
    // Smooth scroll animation logic (target calculation and easing)
    float target_scroll_for_centering = 0;
    if (maxMenuItems > 0 && (maxMenuItems * item_row_h > list_visible_height)) { // Only scroll if content exceeds visible area
        target_scroll_for_centering = (wifiMenuIndex * item_row_h) - (list_visible_height / 2) + (item_row_h / 2);
        // Clamp scroll value
        if (target_scroll_for_centering < 0) target_scroll_for_centering = 0;
        float max_scroll_val = (maxMenuItems * item_row_h) - list_visible_height;
        if (max_scroll_val < 0) max_scroll_val = 0; // Handles case where list is shorter
        if (target_scroll_for_centering > max_scroll_val) {
            target_scroll_for_centering = max_scroll_val;
        }
    }
    // Apply easing to scroll animation
    float scrollDiff = target_scroll_for_centering - currentWifiListScrollOffset_Y_anim;
    if (abs(scrollDiff) > 0.1f) {
        currentWifiListScrollOffset_Y_anim += scrollDiff * GRID_ANIM_SPEED * 0.016f; // Using GRID_ANIM_SPEED, adjust if needed
    } else {
        currentWifiListScrollOffset_Y_anim = target_scroll_for_centering;
    }


    if (!wifiHardwareEnabled) { // If Wi-Fi hardware is globally disabled
        u8g2.setFont(u8g2_font_6x10_tf); 
        u8g2.setDrawColor(1); // White text
        const char* offMsg = getWifiStatusMessage(); // Should be "Wi-Fi Off"
        int msgWidth = u8g2.getStrWidth(offMsg);
        // Center the message vertically and horizontally in the list area
        int text_y_pos = list_start_y_abs + (list_visible_height - (u8g2.getAscent() - u8g2.getDescent())) / 2 + u8g2.getAscent();
        u8g2.drawStr((u8g2.getDisplayWidth() - msgWidth) / 2, text_y_pos, offMsg);
        // No list items to draw if Wi-Fi is off. "Back" handled by physical button.
        // If an "Enable Wi-Fi" item were added, it would be handled by the loop below.
    } else if (wifiIsScanning) { // If Wi-Fi is on and actively scanning
        u8g2.setFont(u8g2_font_6x10_tf); 
        u8g2.setDrawColor(1);
        const char* scanMsg = getWifiStatusMessage(); // Should be "Scanning..."
        int msgWidth = u8g2.getStrWidth(scanMsg);
        int text_y_pos = list_start_y_abs + (list_visible_height - (u8g2.getAscent() - u8g2.getDescent())) / 2 + u8g2.getAscent();
        u8g2.drawStr((u8g2.getDisplayWidth() - msgWidth) / 2, text_y_pos, scanMsg);
    } else { // Wi-Fi is on, not scanning, so draw the list of networks (or "No Networks")
        wifiListAnim.update(); // Update item scale/position animations
        u8g2.setFont(u8g2_font_6x10_tf); // Font for list items

        if (foundWifiNetworksCount == 0 && maxMenuItems <= 2) { // maxMenuItems includes Scan Again & Back
            u8g2.setDrawColor(1);
            const char* noNetMsg = getWifiStatusMessage(); // Should be "No Networks" or similar
            int msgWidth = u8g2.getStrWidth(noNetMsg);
            int text_y_pos = list_start_y_abs + (list_visible_height - (u8g2.getAscent() - u8g2.getDescent())) / 2 + u8g2.getAscent();
            u8g2.drawStr((u8g2.getDisplayWidth() - msgWidth) / 2, text_y_pos, noNetMsg);
            // Still need to draw "Scan Again" and "Back" if they are part of maxMenuItems
            // The loop below will handle them if maxMenuItems is set correctly.
            // If foundWifiNetworksCount is 0, loop for 'i < maxMenuItems' will still run for Scan/Back.
        }


        for (int i = 0; i < maxMenuItems; i++) {
            // Calculate Y position for the current item, considering scroll offset
            int item_center_y_in_full_list = (i * item_row_h) + (item_row_h / 2);
            int item_center_on_screen_y = list_start_y_abs + item_center_y_in_full_list - (int)currentWifiListScrollOffset_Y_anim;
            int item_top_on_screen_y = item_center_on_screen_y - item_row_h / 2;

            // Cull items that are completely off-screen
            if (item_top_on_screen_y + item_row_h < list_start_y_abs || item_top_on_screen_y >= u8g2.getDisplayHeight()) {
                continue;
            }

            bool is_selected_item = (i == wifiMenuIndex);
            
            // Draw selection highlight
            if (is_selected_item) {
                u8g2.setDrawColor(1); // White background for selected item
                drawRndBox(item_padding_x, item_top_on_screen_y, u8g2.getDisplayWidth() - 2 * item_padding_x, item_row_h, 2, true); // Filled
                u8g2.setDrawColor(0); // Black text on selected item
            } else {
                u8g2.setDrawColor(1); // White text for non-selected items
            }
            
            const char* currentItemTextPtr = nullptr;
            bool isNetworkItem = false;
            int8_t currentRssi = 0; 
            bool currentIsSecure = false; 
            int action_icon_type = -1; // For icons like "Scan" or "Back"
            String displayText; // Use String for prepending "*" to connected SSID

            if (i < foundWifiNetworksCount) { // This is a scanned network item
                // Check if this is the currently connected SSID
                if (currentConnectedSsid.length() > 0 && currentConnectedSsid.equals(scannedNetworks[i].ssid)) {
                    displayText = "* "; // Prepend asterisk and space
                    displayText += scannedNetworks[i].ssid;
                } else {
                    displayText = scannedNetworks[i].ssid;
                }
                currentItemTextPtr = displayText.c_str(); // Get char* for drawing functions

                currentRssi = scannedNetworks[i].rssi;
                currentIsSecure = scannedNetworks[i].isSecure;
                isNetworkItem = true;
            } else if (i == foundWifiNetworksCount) { // This is "Scan Again"
                currentItemTextPtr = "Scan Again"; 
                action_icon_type = 16; // Refresh/Rescan icon
            } else if (i == foundWifiNetworksCount + 1) { // This is "Back"
                currentItemTextPtr = "Back"; 
                action_icon_type = 8; // Back arrow icon
            }

            if (currentItemTextPtr == nullptr || currentItemTextPtr[0] == '\0') continue; // Should not happen
            
            // Calculate X positions for icons and text
            int current_content_x = content_padding_x + item_padding_x; // Start X for icons/text
            int icon_y_center_for_draw = item_center_on_screen_y; // Base Y for vertical centering of icons

            if (isNetworkItem) {
                // Draw Wi-Fi signal strength icon (small version)
                drawWifiSignalStrength(current_content_x, icon_y_center_for_draw - 4, currentRssi); // -4 to align 8px icon
                current_content_x += 12; // Approx width of signal icon + spacing
                
                if(currentIsSecure) { // Draw lock icon if secure
                    // Simple lock icon (adjust coordinates as needed for 8px height alignment)
                    int lock_body_start_x = current_content_x; 
                    int body_top_y = icon_y_center_for_draw - 3; // Centering a ~6px high lock
                    u8g2.drawBox(lock_body_start_x, body_top_y, 4, 4); // Lock body 
                    u8g2.drawHLine(lock_body_start_x + 1, body_top_y - 2, 2); // Top of shackle
                    u8g2.drawPixel(lock_body_start_x, body_top_y - 1); 
                    u8g2.drawPixel(lock_body_start_x + 3, body_top_y - 1);
                    current_content_x += 6; // Approx width of lock icon + spacing
                } else { // No lock icon, just add spacing
                    current_content_x += 6; 
                }
            } else if (action_icon_type != -1) { // For "Scan Again" or "Back"
                drawCustomIcon(current_content_x, icon_y_center_for_draw - 4, action_icon_type, false); // Small icon
                current_content_x += 10; // Approx width of small icon + spacing
            } else { // Fallback spacing if no icon
                current_content_x += 10; 
            }

            // Calculate position and available width for text
            int text_x_start = current_content_x;
            int text_available_width = u8g2.getDisplayWidth() - text_x_start - content_padding_x - item_padding_x;
            int text_baseline_y = item_center_on_screen_y - (u8g2.getAscent() + u8g2.getDescent())/2 + u8g2.getAscent();

            // Marquee for selected network SSID, static for others
            if (is_selected_item && isNetworkItem) { // Only marquee for selected network items
                updateMarquee(text_available_width, currentItemTextPtr); 
                if (marqueeActive) {
                    u8g2.drawStr(text_x_start + (int)marqueeOffset, text_baseline_y, marqueeText);
                } else { 
                    char* truncated = truncateText(currentItemTextPtr, text_available_width, u8g2); 
                    u8g2.drawStr(text_x_start, text_baseline_y, truncated); 
                }
            } else { // Non-selected items or non-network items (Scan, Back)
                char* truncated = truncateText(currentItemTextPtr, text_available_width, u8g2);
                u8g2.drawStr(text_x_start, text_baseline_y, truncated);
            }
        }
    }
    u8g2.setDrawColor(1); // Reset draw color
    u8g2.setMaxClipWindow(); // Reset clipping window
}

void drawPasswordInputScreen() {
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setDrawColor(1);

    // Display SSID
    char ssidTitle[40];
    snprintf(ssidTitle, sizeof(ssidTitle), "Enter Pass for:");
    u8g2.drawStr(2, 7 + STATUS_BAR_H, ssidTitle); // Adjusted Y for status bar

    char truncatedSsid[22];
    strncpy(truncatedSsid, currentSsidToConnect, sizeof(truncatedSsid)-1);
    truncatedSsid[sizeof(truncatedSsid)-1] = '\0';
    if (u8g2.getStrWidth(truncatedSsid) > u8g2.getDisplayWidth() - 4) {
        truncateText(currentSsidToConnect, u8g2.getDisplayWidth() - 4, u8g2);
        strncpy(truncatedSsid, SBUF, sizeof(truncatedSsid)-1);
        truncatedSsid[sizeof(truncatedSsid)-1] = '\0';
    }
    u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(truncatedSsid)) / 2, 19 + STATUS_BAR_H, truncatedSsid);


    // Password Input Field
    int fieldX = 5;
    // Assuming status bar is NOT drawn on WIFI_PASSWORD_INPUT screen for more space
    int fieldY = PASSWORD_INPUT_FIELD_Y;
    int fieldW = u8g2.getDisplayWidth() - 10;
    int fieldH = PASSWORD_INPUT_FIELD_H;
    u8g2.drawFrame(fieldX, fieldY, fieldW, fieldH);

    char maskedPassword[PASSWORD_MAX_LEN + 1];
    int len = strlen(wifiPasswordInput);
    for (int i = 0; i < len; ++i) maskedPassword[i] = '*';
    maskedPassword[len] = '\0';

    int textX = fieldX + 3;
    int textY = fieldY + (fieldH - (u8g2.getAscent() - u8g2.getDescent())) / 2 + u8g2.getAscent();
    
    int maxPassDisplayLengthChars = (fieldW - 6) / u8g2.getStrWidth("*");
    if (maxPassDisplayLengthChars < 1) maxPassDisplayLengthChars = 1;

    int passDisplayStartIdx = 0;
    if (wifiPasswordInputCursor >= maxPassDisplayLengthChars) {
        passDisplayStartIdx = wifiPasswordInputCursor - maxPassDisplayLengthChars + 1;
    }
    
    char displaySegment[maxPassDisplayLengthChars + 2];
    strncpy(displaySegment, maskedPassword + passDisplayStartIdx, maxPassDisplayLengthChars);
    displaySegment[maxPassDisplayLengthChars] = '\0';
    u8g2.drawStr(textX, textY, displaySegment);

    int cursorDisplayPosInSegment = wifiPasswordInputCursor - passDisplayStartIdx;
    int cursorX = textX;
    if (cursorDisplayPosInSegment > 0) {
        char tempCursorSubstr[maxPassDisplayLengthChars + 1];
        strncpy(tempCursorSubstr, maskedPassword + passDisplayStartIdx, cursorDisplayPosInSegment);
        tempCursorSubstr[cursorDisplayPosInSegment] = '\0';
        cursorX += u8g2.getStrWidth(tempCursorSubstr);
    }

    if ((millis() / 500) % 2 == 0) {
        u8g2.drawVLine(cursorX, fieldY + 2, fieldH - 4);
    }

    // Instruction Text - with Marquee
    u8g2.setFont(u8g2_font_5x7_tf); // Set font for instruction text
    const char* instrOriginal = "See aux display for keyboard"; // Corrected spelling
    int instrY = u8g2.getDisplayHeight() - 2; // Position at the bottom
    int availableInstrWidth = u8g2.getDisplayWidth() - 4; // Small padding

    // We need a dedicated marquee state for this instruction text
    // For simplicity, let's reuse the global marquee variables.
    // This means only one marquee can be active at a time. If another marquee was active,
    // this will take over.
    updateMarquee(availableInstrWidth, instrOriginal); // updateMarquee uses global marqueeText, marqueeOffset etc.

    if (marqueeActive && strcmp(marqueeText, instrOriginal) == 0) { // Ensure this marquee is the one running
        // Center the marquee text horizontally if it's shorter than available width (when not scrolling yet)
        int text_render_x = (u8g2.getDisplayWidth() - u8g2.getStrWidth(marqueeText)) / 2;
        if (marqueeTextLenPx > availableInstrWidth) { // Only apply offset if text is actually scrolling
             text_render_x = 2 + (int)marqueeOffset; // Apply marquee offset from left padding
        }
        u8g2.drawStr(text_render_x, instrY, marqueeText);
    } else {
        // If not active or a different marquee is running, draw statically (truncated if needed)
        char* truncatedInstr = truncateText(instrOriginal, availableInstrWidth, u8g2);
        u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(truncatedInstr)) / 2, instrY, truncatedInstr);
    }
}

void drawKeyboardOnSmallDisplay() {
    u8g2_small.clearBuffer();
    u8g2_small.setFont(u8g2_font_5x7_tf); // Small font for keys

    const KeyboardKey* currentLayout = getCurrentKeyboardLayout(currentKeyboardLayer);
    if (!currentLayout) return;

    // Define key geometry on the 128x32 display
    const int keyHeight = 8; // 32px / 4 rows
    const int keyWidthBase = 12; // Approx 128px / 10-11 cols, leaves some margin
    const int interKeySpacingX = 1;
    const int interKeySpacingY = 0; // Keys are tight vertically

    int currentX, currentY = 0;

    for (int r = 0; r < KB_ROWS; ++r) {
        currentX = 0;
        currentY = r * keyHeight;
        for (int c = 0; c < KB_LOGICAL_COLS; ) { // c is incremented by colSpan
            const KeyboardKey& key = getKeyFromLayout(currentLayout, r, c);
            if (key.value == KB_KEY_NONE && key.displayChar == 0) { // Skip truly empty placeholder
                 c++; continue;
            }
            if (key.colSpan == 0) { // Also skip colSpan 0 placeholders used for wide keys
                c++; continue;
            }


            int actualKeyWidth = key.colSpan * keyWidthBase + (key.colSpan - 1) * interKeySpacingX;
            if (currentX + actualKeyWidth > u8g2_small.getDisplayWidth()) {
                // This key would overflow, might indicate layout error or need for smaller base width
                actualKeyWidth = u8g2_small.getDisplayWidth() - currentX;
            }
            if (actualKeyWidth <=0) {c += key.colSpan; continue;}


            bool isFocused = (r == keyboardFocusRow && c == keyboardFocusCol);

            if (isFocused) {
                u8g2_small.setDrawColor(1); // White background
                u8g2_small.drawBox(currentX, currentY, actualKeyWidth, keyHeight);
                u8g2_small.setDrawColor(0); // Black text
            } else {
                u8g2_small.setDrawColor(1); // White text/frame
                u8g2_small.drawFrame(currentX, currentY, actualKeyWidth, keyHeight);
            }

            // Determine display char for special keys
            char displayCharStr[2] = {0};
            if (key.displayChar != 0) { // If a specific display char is provided
                displayCharStr[0] = key.displayChar;
            } else { // Fallback for special keys without explicit display char
                switch(key.value) {
                    case KB_KEY_BACKSPACE: displayCharStr[0] = '<'; break;
                    case KB_KEY_ENTER:     displayCharStr[0] = 'E'; break; // Compact 'Ent'
                    case KB_KEY_SHIFT:     displayCharStr[0] = (capsLockActive || (currentKeyboardLayer == KB_LAYER_UPPERCASE && !capsLockActive)) ? 's' : 'S'; break;
                    case KB_KEY_SPACE:     /* Handled by wider box, no char needed or draw "Spc" */ break;
                    case KB_KEY_TO_NUM:    displayCharStr[0] = '1'; break;
                    case KB_KEY_TO_SYM:    displayCharStr[0] = '%'; break;
                    case KB_KEY_TO_LOWER:
                    case KB_KEY_TO_UPPER:  displayCharStr[0] = 'a'; break;
                    default: if (key.value > 0 && key.value < 127) displayCharStr[0] = (char)key.value; break;
                }
            }

            if (displayCharStr[0] != 0) {
                int charWidth = u8g2_small.getStrWidth(displayCharStr);
                int textX = currentX + (actualKeyWidth - charWidth) / 2;
                // For 5x7 font (height 6), in an 8px key, baseline is y + 6 or y + (8-6)/2 + 5 = y+1+5 = y+6
                int textY = currentY + 6;
                u8g2_small.drawStr(textX, textY, displayCharStr);
            }
            currentX += actualKeyWidth + interKeySpacingX;
            c += key.colSpan; // Advance by the number of logical columns this key took
        }
    }
    u8g2_small.setDrawColor(1); // Reset color
    u8g2_small.sendBuffer();
}


void drawWifiConnectingStatusScreen() {
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setDrawColor(1);
    const char* msg1 = "Connecting to:";
    u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(msg1)) / 2, 20, msg1);

    char truncatedSsid[22];
    strncpy(truncatedSsid, currentSsidToConnect, sizeof(truncatedSsid)-1);
    truncatedSsid[sizeof(truncatedSsid)-1] = '\0';
    if (u8g2.getStrWidth(truncatedSsid) > u8g2.getDisplayWidth() - 20) {
        truncateText(currentSsidToConnect, u8g2.getDisplayWidth() - 20, u8g2);
        strncpy(truncatedSsid, SBUF, sizeof(truncatedSsid)-1);
        truncatedSsid[sizeof(truncatedSsid)-1] = '\0';
    }
     u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(truncatedSsid)) / 2, 32, truncatedSsid);


    // Simple animated ellipsis
    int dots = (millis() / 300) % 4;
    char ellipsis[4] = "";
    for(int i=0; i<dots; ++i) ellipsis[i] = '.';
    ellipsis[dots] = '\0';

    char connectingMsg[20];
    snprintf(connectingMsg, sizeof(connectingMsg), "Please wait%s", ellipsis);
    u8g2.drawStr((u8g2.getDisplayWidth() - u8g2.getStrWidth(connectingMsg)) / 2, 50, connectingMsg);
}

void drawWifiConnectionResultScreen() {
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.setDrawColor(1);

    const char* resultMsg = getWifiStatusMessage(); // From wifi_manager

    int msgWidth = u8g2.getStrWidth(resultMsg);
    u8g2.drawStr((u8g2.getDisplayWidth() - msgWidth) / 2, 32, resultMsg);

    WifiConnectionStatus lastStatus = getCurrentWifiConnectionStatus();
    if (lastStatus == WIFI_CONNECTED_SUCCESS) {
        drawCustomIcon(u8g2.getDisplayWidth()/2 - 8, 10, 3, true); // Info icon (example, replace with checkmark if available)
    } else {
        drawCustomIcon(u8g2.getDisplayWidth()/2 - 8, 10, 11, true); // Jamming/Error icon (example, replace with X if available)
    }
}

void drawUI() {
  unsigned long currentTime = millis();

  if (currentMenu == FLASHLIGHT_MODE) {
      selectMux(MUX_CHANNEL_MAIN_DISPLAY);
      u8g2.clearBuffer(); u8g2.setDrawColor(1);
      u8g2.drawBox(0, 0, u8g2.getDisplayWidth(), u8g2.getDisplayHeight());
      u8g2.sendBuffer();
      selectMux(MUX_CHANNEL_SECOND_DISPLAY);
      u8g2_small.clearBuffer(); u8g2_small.setDrawColor(1);
      u8g2_small.drawBox(0, 0, u8g2_small.getDisplayWidth(), u8g2_small.getDisplayHeight());
      u8g2_small.sendBuffer();
      return;
  }

  selectMux(MUX_CHANNEL_SECOND_DISPLAY);
  drawPassiveDisplay(); // This will call drawKeyboardOnSmallDisplay if needed

  selectMux(MUX_CHANNEL_MAIN_DISPLAY);
  u8g2.clearBuffer();
  if (currentMenu != WIFI_PASSWORD_INPUT && currentMenu != WIFI_CONNECTING && currentMenu != WIFI_CONNECTION_INFO) {
      drawStatusBar(); // Status bar might not be wanted on password/connect screens
  }

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
    drawMainMenu();
  } else if (currentMenu == WIFI_SETUP_MENU) {
    drawWifiSetupScreen();
  } else if (currentMenu == WIFI_PASSWORD_INPUT) {
    drawPasswordInputScreen();
  } else if (currentMenu == WIFI_CONNECTING) {
    drawWifiConnectingStatusScreen();
  } else if (currentMenu == WIFI_CONNECTION_INFO) {
    drawWifiConnectionResultScreen();
  }
  else { // Assumed Carousel for Games, Tools (categories), Settings, Utilities
    drawCarouselMenu();
  }
  u8g2.sendBuffer();
}

void drawPassiveDisplay() {
    if (currentMenu == WIFI_PASSWORD_INPUT) {
        drawKeyboardOnSmallDisplay(); // Keyboard takes full small display
    } else {
        // Default passive display content (uptime, mode)
        u8g2_small.clearBuffer();
        u8g2_small.setDrawColor(1);

        // --- Uptime Clock ---
        unsigned long now = millis(); unsigned long allSeconds = now / 1000;
        int runHours = allSeconds / 3600; int secsRemaining = allSeconds % 3600;
        int runMinutes = secsRemaining / 60; int runSeconds = secsRemaining % 60;
        char timeStr[9]; sprintf(timeStr, "%02d:%02d:%02d", runHours, runMinutes, runSeconds);
        u8g2_small.setFont(u8g2_font_helvB08_tr);
        int time_y_baseline = 17; // Adjusted for helvB08 on 32px display
        u8g2_small.drawStr((u8g2_small.getDisplayWidth() - u8g2_small.getStrWidth(timeStr)) / 2, time_y_baseline, timeStr);

        // --- Current Mode Text ---
        const char* modeText = "---";
        // (Logic to determine modeText based on currentMenu as before)
        if (currentMenu == FLASHLIGHT_MODE) modeText = "Torch";
        else if (currentMenu == MAIN_MENU && menuIndex < getMainMenuItemsCount()) modeText = mainMenuItems[menuIndex];
        else if (currentMenu == UTILITIES_MENU && menuIndex < getUtilitiesMenuItemsCount()) modeText = utilitiesMenuItems[menuIndex];
        else if (currentMenu == GAMES_MENU) modeText = "Games";
        else if (currentMenu == TOOLS_MENU) modeText = "Tools";
        else if (currentMenu == SETTINGS_MENU) modeText = "Settings";
        else if (currentMenu == TOOL_CATEGORY_GRID && toolsCategoryIndex < (getToolsMenuItemsCount() -1) ) modeText = toolsMenuItems[toolsCategoryIndex];
        else if (currentMenu == WIFI_SETUP_MENU) {
            if (wifiIsScanning) modeText = "Scanning WiFi";
            else modeText = "Wi-Fi Setup";
        } else if (currentMenu == WIFI_CONNECTING) {
            modeText = "Connecting...";
        } else if (currentMenu == WIFI_CONNECTION_INFO) {
            modeText = getWifiStatusMessage(); // Show connection result
        }


        u8g2_small.setFont(u8g2_font_6x12_tr); // Or other suitable font
        char truncatedModeText[22]; strncpy(truncatedModeText, modeText, sizeof(truncatedModeText)-1);
        truncatedModeText[sizeof(truncatedModeText)-1] = '\0';
        if (u8g2_small.getStrWidth(truncatedModeText) > u8g2_small.getDisplayWidth() - 4) {
            truncateText(modeText, u8g2_small.getDisplayWidth() - 4, u8g2_small); // SBUF used
            strncpy(truncatedModeText, SBUF, sizeof(truncatedModeText)-1);
            truncatedModeText[sizeof(truncatedModeText)-1] = '\0';
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
  // SBUF is used here. Ensure it's large enough.
  int originalLen = strlen(originalText);
  if (originalLen == 0) {
    SBUF[0] = '\0';
    return SBUF;
  }

  // Copy original to SBUF, ensuring space for "..." and null terminator
  // Max text part length = sizeof(SBUF) - 1 (for null) - 3 (for "...")
  int maxTextPartLen = sizeof(SBUF) - 4;
  if (maxTextPartLen < 1) { // Not enough space in SBUF even for one char + "..."
      SBUF[0] = '\0';
      return SBUF;
  }

  strncpy(SBUF, originalText, maxTextPartLen);
  SBUF[maxTextPartLen] = '\0'; // Null-terminate the (potentially) shortened text part

  if (display.getUTF8Width(SBUF) <= maxWidthPixels) {
    // If the (possibly already shortened to fit SBUF) text fits, return it.
    // We need to ensure the original full string wasn't too long for SBUF initially.
    if (originalLen <= maxTextPartLen) {
        // Original string fit SBUF entirely, and it also fits maxWidthPixels
        strncpy(SBUF, originalText, sizeof(SBUF)-1); // Recopy full original
        SBUF[sizeof(SBUF)-1] = '\0';
        return SBUF;
    }
    // else, SBUF already contains the maxTextPartLen portion, which fits.
    return SBUF;
  }

  // If it doesn't fit, append "..." and shorten until it does
  // SBUF currently holds up to maxTextPartLen of originalText
  strcat(SBUF, "..."); // This is safe because we reserved space

  while (display.getUTF8Width(SBUF) > maxWidthPixels && strlen(SBUF) > 3) { // Ensure "..." itself isn't removed
    SBUF[strlen(SBUF) - 4] = '\0'; // Remove char before "..."
    strcat(SBUF, "...");
  }

  // Final check if even "..." or parts of it are too wide
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
    u8g2.setFont(u8g2_font_6x10_tf); 
    const char* titleText = "Kiva OS"; 

    // --- Title Logic (remains the same) ---
    if (currentMenu == TOOL_CATEGORY_GRID) { /* ... */ }
    else if (currentMenu == WIFI_SETUP_MENU) { /* ... */ }
    else if (currentMenu != MAIN_MENU && mainMenuSavedIndex >= 0 && mainMenuSavedIndex < getMainMenuItemsCount()) { /* ... */ }
    // (Ensure full title logic is present from previous versions)
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
    int max_title_width = 65; 
     if (u8g2.getStrWidth(titleBuffer) > max_title_width) { 
        truncateText(titleText, max_title_width - 5, u8g2); 
        strncpy(titleBuffer, SBUF, sizeof(titleBuffer)-1); 
        titleBuffer[sizeof(titleBuffer) - 1] = '\0';
    }
    u8g2.drawStr(2, 7, titleBuffer); 

    // --- Battery Display Area (Right Aligned) ---
    int battery_area_right_margin = 2;
    int current_x_origin_for_battery = u8g2.getDisplayWidth() - battery_area_right_margin;

    // 1. Draw Battery Icon (Simplified)
    int bat_icon_y_top = 3; 
    int bat_icon_width = 9;  // From drawBatIcon: body 7 + tip 1 + 1px spacing between = 9 approx
    current_x_origin_for_battery -= bat_icon_width; 
    drawBatIcon(current_x_origin_for_battery, bat_icon_y_top, s);

    // 2. Draw Charging Icon (if charging) - Using Custom Icon 17
    if (isCharging) {
        int charge_icon_width = 6; // Bounding box for our custom small lightning bolt
        int charge_icon_height = 7; // Bounding box height
        int charge_icon_spacing = 2;
        current_x_origin_for_battery -= (charge_icon_width + charge_icon_spacing); 
        
        // Vertically align the custom icon. If bat_icon_y_top = 3 and icon is 7px high,
        // and status bar font has baseline at y=7.
        // For custom icon, y is its top-left.
        // If bat icon top is 3, and it's 5px high, its center is ~5.5.
        // If lightning icon is 7px high, to center it with bat icon, its top should be (3 + 5/2) - 7/2 = 3 - 1 = 2
        int charge_icon_y_top = bat_icon_y_top -1; // Aim to make it look vertically centered with bat icon
                                                 // (bat icon is 5px high, lightning 7px high. Difference is 2. Shift by 1)
        if (charge_icon_y_top < 0) charge_icon_y_top = 0; // Don't draw above status bar


        // Ensure draw color is set for the icon (white on black, or black on white if status bar inverted)
        // Assuming status bar background is black, icon should be white.
        u8g2.setDrawColor(1); // Set to white if not already
        drawCustomIcon(current_x_origin_for_battery, charge_icon_y_top, 17, false); 
    }

    // 3. Draw Battery Percentage Text
    char batteryStr[6]; 
    snprintf(batteryStr, sizeof(batteryStr), "%u%%", s);
    int percent_text_width = u8g2.getStrWidth(batteryStr);
    int percent_text_spacing = 2;
    current_x_origin_for_battery -= (percent_text_width + percent_text_spacing); 
    // Ensure draw color is set for text (white on black)
    u8g2.setDrawColor(1);
    u8g2.drawStr(current_x_origin_for_battery, 7, batteryStr); 

    // --- Status Bar Line ---
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
    
    // Determine icon type based on item text for main menu
    int iconType = 0; // Default/fallback icon
    if (strcmp(itms[i], "Tools") == 0) iconType = 0;
    else if (strcmp(itms[i], "Games") == 0) iconType = 1;
    else if (strcmp(itms[i], "Settings") == 0) iconType = 2;
    else if (strcmp(itms[i], "Utilities") == 0) iconType = 21; // <--- NEW ICON TYPE FOR UTILITIES
    else if (strcmp(itms[i], "Info") == 0) iconType = 3;
    
    drawCustomIcon(icon_x_pos, icon_y_pos, iconType, true); // Main menu uses large icons

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
  const int icon_render_size = 16; // For large icons in carousel cards
  const int icon_margin_top = 3; 
  const int text_margin_from_icon = 2; 

  // Set font for selected item text here, as updateMarquee relies on current font for width
  u8g2.setFont(u8g2_font_6x10_tf); 
  
  for (int i = 0; i < maxMenuItems; i++) {
    float current_item_scale = subMenuAnim.itemScale[i];
    if (current_item_scale <= 0.05f) continue; // Skip if too small to be visible

    float current_item_offset_x = subMenuAnim.itemOffsetX[i];
    int card_w_scaled = (int)(CarouselAnimation().cardBaseW * current_item_scale); // Access const from type
    int card_h_scaled = (int)(CarouselAnimation().cardBaseH * current_item_scale);
    int card_x_abs = screen_center_x_abs + (int)current_item_offset_x - (card_w_scaled / 2);
    int card_y_abs = carousel_center_y_abs - (card_h_scaled / 2);
    bool is_selected_item = (i == menuIndex);

    const char** current_item_list_ptr = nullptr;
    int icon_type_for_item = 0; 
    
    // Declare these variables here, before they are used in the UTILITIES_MENU case
    char itemDisplayTextBuffer[25]; // Buffer for dynamic text like "Vibration: ON" (increased size)
    const char* itemTextToDisplay = nullptr;


    // Determine item list, text, and icon type based on currentMenu
    if (currentMenu == GAMES_MENU) {
      current_item_list_ptr = gamesMenuItems;
      itemTextToDisplay = current_item_list_ptr[i]; // Standard item text
      if (i == 0) icon_type_for_item = 4; else if (i == 1) icon_type_for_item = 5; 
      else if (i == 2) icon_type_for_item = 6; else if (i == 3) icon_type_for_item = 7; 
      else if (i == 4 && i < getGamesMenuItemsCount()) icon_type_for_item = 8; // Back
      else continue;

    } else if (currentMenu == TOOLS_MENU) {
      current_item_list_ptr = toolsMenuItems;
      itemTextToDisplay = current_item_list_ptr[i];
      if (i == 0) icon_type_for_item = 12; else if (i == 1) icon_type_for_item = 9;  
      else if (i == 2) icon_type_for_item = 10; else if (i == 3) icon_type_for_item = 9;  
      else if (i == 4) icon_type_for_item = 11; else if (i == 5 && i < getToolsMenuItemsCount()) icon_type_for_item = 8; // Back  
      else continue;

    } else if (currentMenu == SETTINGS_MENU) {
      current_item_list_ptr = settingsMenuItems; 
      itemTextToDisplay = current_item_list_ptr[i];
      if (i == 0) icon_type_for_item = 9;   
      else if (i == 1) icon_type_for_item = 13; 
      else if (i == 2) icon_type_for_item = 14;  
      else if (i == 3) icon_type_for_item = 15;  
      else if (i == 4) icon_type_for_item = 3;   
      else if (i == 5 && i < getSettingsMenuItemsCount()) icon_type_for_item = 8; // Back   
      else continue;

    } else if (currentMenu == UTILITIES_MENU) {
        current_item_list_ptr = utilitiesMenuItems;
        // Default to the static item name, will be overridden for dynamic items
        itemTextToDisplay = current_item_list_ptr[i]; 

        if (strcmp(itemTextToDisplay, "Vibration") == 0) {
            icon_type_for_item = 18; // Vibration icon
            snprintf(itemDisplayTextBuffer, sizeof(itemDisplayTextBuffer), "Vibration: %s", vibrationOn ? "ON" : "OFF");
            itemTextToDisplay = itemDisplayTextBuffer;
        } else if (strcmp(itemTextToDisplay, "Laser") == 0) {
            icon_type_for_item = 19; // Laser icon
            snprintf(itemDisplayTextBuffer, sizeof(itemDisplayTextBuffer), "Laser: %s", laserOn ? "ON" : "OFF");
            itemTextToDisplay = itemDisplayTextBuffer;
        } else if (strcmp(itemTextToDisplay, "Flashlight") == 0) {
            icon_type_for_item = 20; // Flashlight icon
            // No dynamic text needed, itemTextToDisplay remains "Flashlight"
        } else if (strcmp(itemTextToDisplay, "Back") == 0) {
            icon_type_for_item = 8; // Back icon
            // No dynamic text needed
        } else {
            continue; // Should not happen with defined utilitiesMenuItems
        }
    } else {
      continue; // Unknown menu type for carousel
    }

    if (card_w_scaled <= 0 || card_h_scaled <= 0) continue; // Skip if card is invisible

    if (is_selected_item) {
      u8g2.setDrawColor(1); // White background for selected card
      drawRndBox(card_x_abs, card_y_abs, card_w_scaled, card_h_scaled, 3, true); // Filled rounded box

      u8g2.setDrawColor(0); // Black color for content (icon and text) on selected card
      int icon_x_pos = card_x_abs + (card_w_scaled - icon_render_size) / 2;
      int icon_y_pos = card_y_abs + icon_margin_top;
      drawCustomIcon(icon_x_pos, icon_y_pos, icon_type_for_item, true); // Carousel uses large icons

      // Font for selected item text is already u8g2_font_6x10_tf (set before loop)
      int text_area_y_start_abs = card_y_abs + icon_margin_top + icon_render_size + text_margin_from_icon;
      int text_area_h_available = card_h_scaled - (text_area_y_start_abs - card_y_abs) - card_internal_padding;
      int text_baseline_y_render = text_area_y_start_abs + (text_area_h_available - (u8g2.getAscent() - u8g2.getDescent())) / 2 + u8g2.getAscent() -1;


      int card_inner_content_width = card_w_scaled - 2 * card_internal_padding;
      updateMarquee(card_inner_content_width, itemTextToDisplay); 

      // Clip window for text rendering inside the card
      int clip_x1 = card_x_abs + card_internal_padding;
      int clip_y1 = text_area_y_start_abs; 
      int clip_x2 = card_x_abs + card_w_scaled - card_internal_padding;
      int clip_y2 = card_y_abs + card_h_scaled - card_internal_padding; 

      clip_y1 = max(clip_y1, card_y_abs); 
      clip_y2 = min(clip_y2, card_y_abs + card_h_scaled);

      if (clip_x1 < clip_x2 && clip_y1 < clip_y2) { // Valid clip area
        u8g2.setClipWindow(clip_x1, clip_y1, clip_x2, clip_y2);
        if (marqueeActive) {
          u8g2.drawStr(card_x_abs + card_internal_padding + (int)marqueeOffset, text_baseline_y_render, marqueeText);
        } else {
          int text_width_pixels = u8g2.getStrWidth(itemTextToDisplay);
          u8g2.drawStr(card_x_abs + (card_w_scaled - text_width_pixels) / 2, text_baseline_y_render, itemTextToDisplay);
        }
        u8g2.setMaxClipWindow(); // Reset clip after drawing text
      }
    } else { // Not selected item
      u8g2.setDrawColor(1); // White color for frame and text
      drawRndBox(card_x_abs, card_y_abs, card_w_scaled, card_h_scaled, 2, false); // Frame only

      if (current_item_scale > 0.5) { // Only draw text if item is reasonably scaled
        u8g2.setFont(u8g2_font_5x7_tf); // Smaller font for non-selected items' text
        char* display_text_truncated = truncateText(itemTextToDisplay, card_w_scaled - 2 * card_internal_padding, u8g2);
        int text_width_pixels = u8g2.getStrWidth(display_text_truncated);
        // Center baseline for 5x7 font
        int text_baseline_y_render = card_y_abs + (card_h_scaled - (u8g2.getAscent() - u8g2.getDescent())) / 2 + u8g2.getAscent();
        u8g2.drawStr(card_x_abs + (card_w_scaled - text_width_pixels) / 2, text_baseline_y_render, display_text_truncated);
        u8g2.setFont(u8g2_font_6x10_tf); // Reset to main carousel font for next iteration (selected item)
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
    
    case 17: // UI Icon: Small Lightning Bolt (for charging status) - Refined
      // Target: Small, sharp, modern. Approx 5-6px wide, 7-8px high.
      // 'isLarge' can provide a slightly scaled up version if needed elsewhere.
      if (isLarge) { // Larger version (e.g., for a different UI context)
        // Example for a ~10px wide, ~14px high bolt
        u8g2.drawLine(x + 5, y + 0, x + 2, y + 6);  // Top-left stroke
        u8g2.drawLine(x + 2, y + 6, x + 7, y + 5);  // Middle-right stroke (crossing)
        u8g2.drawLine(x + 7, y + 5, x + 4, y + 13); // Bottom-left stroke
      } else { // Small version for status bar (target ~5px width, ~7px height)
        // Simple, sharp "N" or "Z" shape
        // x, y is top-left of the drawing bounding box.
        // Let's aim for a 5x7 icon bounding box.
        // y is y_top of this box.

        // Top point: (x+2, y)
        // Mid-left: (x+0, y+3)
        // Mid-right: (x+4, y+3)
        // Bottom point: (x+2, y+6)

        u8g2.drawLine(x + 2, y + 0, x + 0, y + 3); // Down-left
        u8g2.drawLine(x + 0, y + 3, x + 4, y + 3); // Across to right
        u8g2.drawLine(x + 4, y + 3, x + 2, y + 6); // Down-left to bottom point

        // Alternative slightly more "jagged" Z:
        // u8g2.drawLine(x + 3, y + 0, x + 0, y + 2); // Top part, slanted
        // u8g2.drawLine(x + 0, y + 2, x + 4, y + 4); // Middle part, slanted
        // u8g2.drawLine(x + 4, y + 4, x + 1, y + 6); // Bottom part, slanted
      }
      break;
    
    case 18: // UI Icon: Vibration Motor (e.g., eccentric rotating mass symbol)
      if (isLarge) {
        u8g2.drawCircle(x + 8, y + 8, 6); // Outer circle
        u8g2.drawBox(x + 8 - 1, y + 8 - 4, 2, 3); // Offset mass part 1
        u8g2.drawDisc(x+8, y+8, 1); // Center dot
      } else { // Small
        u8g2.drawCircle(x + 4, y + 4, 3);
        u8g2.drawPixel(x + 4, y + 1); // Small indication of offset
        u8g2.drawPixel(x+4, y+4);
      }
      break;
    case 19: // UI Icon: Laser Beam (e.g., starburst or lines)
      if (isLarge) {
        u8g2.drawDisc(x + 8, y + 8, 2); // Center point
        for (int k = 0; k < 8; k++) {
          float angle = k * PI / 4.0;
          u8g2.drawLine(x + 8 + round(cos(angle) * 3), y + 8 + round(sin(angle) * 3),
                         x + 8 + round(cos(angle) * 7), y + 8 + round(sin(angle) * 7));
        }
      } else { // Small
        u8g2.drawDisc(x + 4, y + 4, 1);
        u8g2.drawLine(x + 4, y + 0, x + 4, y + 7);
        u8g2.drawLine(x + 0, y + 4, x + 7, y + 4);
        u8g2.drawLine(x + 1, y + 1, x + 6, y + 6);
        u8g2.drawLine(x + 1, y + 6, x + 6, y + 1);
      }
      break;
    case 20: // UI Icon: Flashlight (e.g., stylized light beam)
      if (isLarge) {
        u8g2.drawTriangle(x + 4, y + 2, x + 12, y + 2, x + 8, y + 0); // Top of lamp
        u8g2.drawBox(x + 6, y + 2, 4, 5); // Lamp body
        // Beams
        u8g2.drawLine(x + 2, y + 8, x + 6, y + 14);
        u8g2.drawLine(x + 8, y + 8, x + 8, y + 14);
        u8g2.drawLine(x + 14, y + 8, x + 10, y + 14);
      } else { // Small
        u8g2.drawBox(x + 2, y + 0, 4, 3); // Lamp head
        u8g2.drawLine(x+0,y+4, x+3,y+7);
        u8g2.drawLine(x+4,y+4, x+4,y+7);
        u8g2.drawLine(x+7,y+4, x+5,y+7);
      }
      break;
    
    case 21: // UI Icon: Utilities (e.g., simple toolbox or multi-tool)
      if (isLarge) { // 16x16
        // Simple toolbox
        u8g2.drawRFrame(x + 2, y + 5, 12, 8, 1); // Main box
        u8g2.drawBox(x + 1, y + 3, 14, 2);     // Lid / Top part
        u8g2.drawRFrame(x + 6, y + 1, 4, 3, 1); // Handle
        u8g2.drawPixel(x+4, y+8); // latch/dot
        u8g2.drawPixel(x+11, y+8); // latch/dot
      } else { // 8x8
        u8g2.drawFrame(x + 1, y + 3, 6, 4); // Box
        u8g2.drawHLine(x + 0, y + 2, 8);   // Lid
        u8g2.drawFrame(x + 3, y + 0, 2, 2); // Handle
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