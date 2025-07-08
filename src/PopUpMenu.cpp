#include "PopUpMenu.h" // <-- Renamed include
#include "App.h"
#include "UI_Utils.h"
#include <Arduino.h>
#include <algorithm> // for std::max, std::min

// --- Renamed class and constructor ---
PopUpMenu::PopUpMenu() :
    selectedOption_(0),
    overlayScale_(0.0f)
{
}

void PopUpMenu::configure(std::string title, std::string message, OnConfirmCallback onConfirm, std::string confirmText, std::string cancelText) {
    title_ = title;
    message_ = message;
    onConfirm_ = onConfirm;
    confirmText_ = confirmText;
    cancelText_ = cancelText;
}

void PopUpMenu::onEnter(App* app) {
    selectedOption_ = 0; // Default to cancel
    overlayScale_ = 0.0f;
}

void PopUpMenu::onUpdate(App* app) {
    // Animate the scale-in effect
    float targetScale = 1.0f;
    float diff = targetScale - overlayScale_;
    if (abs(diff) > 0.01f) {
        overlayScale_ += diff * (GRID_ANIM_SPEED * 1.5f) * 0.016f; // A bit faster than grid
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
}

void PopUpMenu::handleInput(App* app, InputEvent event) {
    switch(event) {
        case InputEvent::ENCODER_CW:
        case InputEvent::ENCODER_CCW:
        case InputEvent::BTN_LEFT_PRESS:
        case InputEvent::BTN_RIGHT_PRESS:
            selectedOption_ = 1 - selectedOption_; // Toggle between 0 and 1
            break;

        case InputEvent::BTN_ENCODER_PRESS:
        case InputEvent::BTN_OK_PRESS:
            if (selectedOption_ == 1) { // Confirm
                if (onConfirm_) {
                    onConfirm_(app);
                }
            }
            // Whether confirmed or cancelled, go back.
            app->changeMenu(MenuType::BACK);
            break;

        case InputEvent::BTN_BACK_PRESS:
            // Back button always cancels.
            app->changeMenu(MenuType::BACK);
            break;

        default:
            break;
    }
}

const char* PopUpMenu::getTitle() const {
    // The pop-up menu doesn't have a static title for the status bar.
    // The title is drawn inside the overlay itself.
    return "Confirm";
}

void PopUpMenu::draw(App* app, U8G2& display) {
    // --- Draw the pop-up overlay on top of the existing buffer ---
    if (overlayScale_ <= 0.05f) return;

    int baseW = 110;
    int baseH = 55;

    int w = (int)(baseW * overlayScale_);
    int h = (int)(baseH * overlayScale_);
    if (w < 1 || h < 1) return;

    int x = (display.getDisplayWidth() - w) / 2;
    int y = (display.getDisplayHeight() - h) / 2;
    y = std::max(y, (int)STATUS_BAR_H + 3);

    // Draw the box
    display.setDrawColor(1);
    drawRndBox(display, x, y, w, h, 3, true);
    display.setDrawColor(0);
    display.drawRFrame(x, y, w, h, 3);

    // Only draw contents when fully scaled
    if (overlayScale_ < 0.95f) return;

    display.setDrawColor(0); // Text inside is inverse color
    int padding = 4;

    // Title
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr(x + (w - display.getStrWidth(title_.c_str())) / 2, y + padding + display.getAscent(), title_.c_str());

    // Message
    char* truncatedMsg = truncateText(message_.c_str(), w - 2 * padding, display);
    display.drawStr(x + (w - display.getStrWidth(truncatedMsg)) / 2, y + 28, truncatedMsg);

    // Options
    int opt_y_base = y + h - padding - 2;
    int opt_h = 12;
    int opt_y = opt_y_base - opt_h;

    int opt0_w = display.getStrWidth(cancelText_.c_str()) + 8;
    int opt1_w = display.getStrWidth(confirmText_.c_str()) + 8;
    int total_w = opt0_w + opt1_w + 10;
    int start_x = x + (w - total_w) / 2;

    // Cancel Option (0)
    if (selectedOption_ == 0) {
        drawRndBox(display, start_x, opt_y, opt0_w, opt_h, 2, true); // Filled box
        display.setDrawColor(1); // Inverted text
    } else {
        drawRndBox(display, start_x, opt_y, opt0_w, opt_h, 2, false); // Frame only
        display.setDrawColor(0); // Normal text
    }
    display.drawStr(start_x + 4, opt_y + 9, cancelText_.c_str());

    // Confirm Option (1)
    display.setDrawColor(0); // Reset color
    int opt1_x = start_x + opt0_w + 10;
    if (selectedOption_ == 1) {
        drawRndBox(display, opt1_x, opt_y, opt1_w, opt_h, 2, true); // Filled
        display.setDrawColor(1); // Inverted text
    } else {
        drawRndBox(display, opt1_x, opt_y, opt1_w, opt_h, 2, false); // Frame
        display.setDrawColor(0); // Normal text
    }
    display.drawStr(opt1_x + 4, opt_y + 9, confirmText_.c_str());

    display.setDrawColor(1); // Reset draw color for next frame
}