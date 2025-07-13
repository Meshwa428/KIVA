#include "PopUpMenu.h"
#include "App.h"
#include "UI_Utils.h"
#include <Arduino.h>
#include <algorithm> // for std::max, std::min

PopUpMenu::PopUpMenu() :
    onConfirm_(nullptr),
    executeOnConfirmBeforeExit_(false),
    selectedOption_(0),
    overlayScale_(0.0f)
{
}

void PopUpMenu::configure(std::string title, std::string message, OnConfirmCallback onConfirm, std::string confirmText, std::string cancelText, bool executeOnConfirmBeforeExit) {
    title_ = title;
    message_ = message;
    onConfirm_ = onConfirm;
    confirmText_ = confirmText;
    cancelText_ = cancelText;
    executeOnConfirmBeforeExit_ = executeOnConfirmBeforeExit;
}

void PopUpMenu::onEnter(App* app) {
    // If there is no "Cancel" button, default the selection to "Confirm" (option 1).
    // Otherwise, default to "Cancel" (option 0).
    bool hasCancelButton = !cancelText_.empty();
    selectedOption_ = hasCancelButton ? 0 : 1;
    overlayScale_ = 0.0f;
}

void PopUpMenu::onUpdate(App* app) {
    // Animate the scale-in effect
    float targetScale = 1.0f;
    float diff = targetScale - overlayScale_;
    if (abs(diff) > 0.01f) {
        overlayScale_ += diff * (GRID_ANIM_SPEED * 1.5f) * 0.016f;
    } else {
        overlayScale_ = targetScale;
    }
}

void PopUpMenu::onExit(App* app) {
    // Clear the configuration to prevent old data from being used
    title_ = "";
    message_ = "";
    onConfirm_ = nullptr;
    overlayScale_ = 0.0f;
    executeOnConfirmBeforeExit_ = false;
}

void PopUpMenu::handleInput(App* app, InputEvent event) {
    bool hasCancelButton = !cancelText_.empty();

    switch(event) {
        case InputEvent::ENCODER_CW:
        case InputEvent::ENCODER_CCW:
        case InputEvent::BTN_LEFT_PRESS:
        case InputEvent::BTN_RIGHT_PRESS:
            // Only allow toggling if there are two buttons.
            if (hasCancelButton) {
                selectedOption_ = 1 - selectedOption_; // Toggle between 0 (Cancel) and 1 (Confirm)
            }
            break;

        case InputEvent::BTN_ENCODER_PRESS:
        case InputEvent::BTN_OK_PRESS:
            if (selectedOption_ == 1) { // Confirm button pressed
                if (onConfirm_) {
                    // Execute the callback first.
                    onConfirm_(app); 

                    // If the callback is NOT responsible for navigating away,
                    // then the popup must dismiss itself. This is for simple confirm dialogs.
                    if (executeOnConfirmBeforeExit_) {
                        app->changeMenu(MenuType::BACK);
                    }
                    // If executeOnConfirmBeforeExit_ is false, we assume the callback
                    // handled navigation, so the pop-up does nothing more, preventing a double-navigation issue.
                } else {
                    // If there's no callback, OK just acts like Cancel/Back.
                    app->changeMenu(MenuType::BACK);
                }
            } else { // Cancel button pressed (only possible if hasCancelButton is true)
                app->changeMenu(MenuType::BACK); // Dismiss the popup
            }
            break;

        case InputEvent::BTN_BACK_PRESS:
            app->changeMenu(MenuType::BACK);
            break;

        default:
            break;
    }
}

const char* PopUpMenu::getTitle() const {
    return title_.c_str();
}

void PopUpMenu::draw(App* app, U8G2& display) {
    if (overlayScale_ <= 0.05f) return;

    int baseW = 110;
    int baseH = 52; // Reduced height to ensure it fits below status bar

    int w = (int)(baseW * overlayScale_);
    int h = (int)(baseH * overlayScale_);
    if (w < 1 || h < 1) return;

    int x = (display.getDisplayWidth() - w) / 2;

    // This calculation correctly centers the popup in the available space
    // *below* the status bar, preventing it from being cut off.
    int drawable_area_y_start = STATUS_BAR_H + 1; // Start 1px below the status bar line
    int drawable_area_height = display.getDisplayHeight() - drawable_area_y_start;
    int y = drawable_area_y_start + (drawable_area_height - h) / 2;

    display.setDrawColor(1);
    drawRndBox(display, x, y, w, h, 3, true);
    display.setDrawColor(0);
    display.drawRFrame(x, y, w, h, 3);

    if (overlayScale_ < 0.95f) return;

    display.setDrawColor(0);
    int padding = 4;

    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(x + (w - display.getStrWidth(title_.c_str())) / 2, y + padding + display.getAscent(), title_.c_str());

    // Use wrapped text for the message
    std::vector<const uint8_t*> fonts = {u8g2_font_6x10_tf, u8g2_font_5x7_tf};
    drawWrappedText(display, message_.c_str(), x + padding, y + 16, w - 2 * padding, 22, fonts);

    // --- BUTTON DRAWING LOGIC ---
    display.setFont(u8g2_font_6x10_tf);
    // --- MODIFIED: Adjusted opt_y_base to move buttons down by 2px ---
    int opt_y_base = y + h - padding;
    int opt_h = 12;
    int opt_y = opt_y_base - opt_h;

    bool hasCancelButton = !cancelText_.empty();

    if (hasCancelButton) {
        // --- Two-button logic (for "OK/Cancel" dialogs) ---
        int opt0_w = display.getStrWidth(cancelText_.c_str()) + 8;
        int opt1_w = display.getStrWidth(confirmText_.c_str()) + 8;
        int total_w = opt0_w + opt1_w + 10;
        int start_x = x + (w - total_w) / 2;

        // Draw Cancel Button
        if (selectedOption_ == 0) {
            drawRndBox(display, start_x, opt_y, opt0_w, opt_h, 2, true);
            display.setDrawColor(1);
        } else {
            drawRndBox(display, start_x, opt_y, opt0_w, opt_h, 2, false);
            display.setDrawColor(0);
        }
        display.drawStr(start_x + 4, opt_y + 9, cancelText_.c_str());

        // Draw Confirm Button
        display.setDrawColor(0);
        int opt1_x = start_x + opt0_w + 10;
        if (selectedOption_ == 1) {
            drawRndBox(display, opt1_x, opt_y, opt1_w, opt_h, 2, true);
            display.setDrawColor(1);
        } else {
            drawRndBox(display, opt1_x, opt_y, opt1_w, opt_h, 2, false);
            display.setDrawColor(0);
        }
        display.drawStr(opt1_x + 4, opt_y + 9, confirmText_.c_str());

    } else {
        // --- One-button logic (for "OK" dialogs) ---
        int opt1_w = display.getStrWidth(confirmText_.c_str()) + 8;
        int opt1_x = x + (w - opt1_w) / 2; // Center the single button

        // The only option is "confirm", so it's always selected.
        drawRndBox(display, opt1_x, opt_y, opt1_w, opt_h, 2, true);
        display.setDrawColor(1); // The text color is inverted (white on black bg)
        display.drawStr(opt1_x + 4, opt_y + 9, confirmText_.c_str());
    }

    display.setDrawColor(1);
}