// KIVA/src/BrightnessMenu.cpp

#include "Event.h"
#include "EventDispatcher.h"
#include "BrightnessMenu.h"
#include "App.h"
#include "UI_Utils.h"
#include "ConfigManager.h"

BrightnessMenu::BrightnessMenu() : 
    selectedIndex_(0),
    isAdjusting_(false), // Initialize new members
    adjustDirection_(0),
    pressStartTime_(0),
    lastAdjustTime_(0)
{}

void BrightnessMenu::onEnter(App* app, bool isForwardNav) {
    EventDispatcher::getInstance().subscribe(EventType::APP_INPUT, this);
    if (isForwardNav) {
        selectedIndex_ = 0;
    }
    isAdjusting_ = false; // Reset state on enter
    animation_.resize(2);
    animation_.init();
    animation_.setTargets(selectedIndex_, 2);
}

void BrightnessMenu::onUpdate(App* app) {
    if (animation_.update()) {
        app->requestRedraw();
    }

    if (isAdjusting_) {
        unsigned long currentTime = millis();
        unsigned long holdDuration = currentTime - pressStartTime_;
        
        // Define repeat intervals and step sizes for different stages of acceleration
        unsigned long repeatInterval;
        int stepSize;

        if (holdDuration > 1500) {
            // Stage 3: Maximum Boost
            repeatInterval = 40;  // Very fast updates
            stepSize = 5;         // Large jumps
        } else if (holdDuration > 500) {
            // Stage 2: Medium Speed
            repeatInterval = 80;  // Fast updates
            stepSize = 2;         // Medium jumps
        } else {
            // Stage 1: Initial Slow Speed (for fine-tuning after first press)
            repeatInterval = 100; // Slow updates
            stepSize = 1;         // Small jumps
        }

        if (currentTime - lastAdjustTime_ > repeatInterval) {
            changeBrightness(app, adjustDirection_ * stepSize); 
            lastAdjustTime_ = currentTime;
        }
    }
}

void BrightnessMenu::onExit(App* app) {
    EventDispatcher::getInstance().unsubscribe(EventType::APP_INPUT, this);
    isAdjusting_ = false; // Ensure state is cleared on exit
    
    // The user is done adjusting. Now, we commit the final value to storage.
    app->getConfigManager().saveSettings();
}

void BrightnessMenu::handleInput(InputEvent event, App* app) {
    // Handle starting the adjustment
    if (event == InputEvent::BTN_LEFT_PRESS || event == InputEvent::ENCODER_CCW) {
        // --- THIS IS THE FIX ---
        // If we are already in an adjustment state, ignore new press events.
        // This prevents the HardwareManager's repeater from resetting our timer.
        if (isAdjusting_) return;

        isAdjusting_ = true;
        adjustDirection_ = -1;
        pressStartTime_ = millis();
        lastAdjustTime_ = millis();
        changeBrightness(app, adjustDirection_ * 1); // Immediate change of 1 on first press
        return;
    }
    if (event == InputEvent::BTN_RIGHT_PRESS || event == InputEvent::ENCODER_CW) {
        // --- THIS IS THE FIX ---
        if (isAdjusting_) return;

        isAdjusting_ = true;
        adjustDirection_ = 1;
        pressStartTime_ = millis();
        lastAdjustTime_ = millis();
        changeBrightness(app, adjustDirection_ * 1); // Immediate change of 1 on first press
        return;
    }

    // Handle stopping the adjustment
    if (event == InputEvent::BTN_LEFT_RELEASE || event == InputEvent::BTN_RIGHT_RELEASE) {
        isAdjusting_ = false;
        adjustDirection_ = 0;
        return;
    }

    // Handle switching between items (remains the same)
    switch(event) {
        case InputEvent::BTN_UP_PRESS:
        case InputEvent::BTN_DOWN_PRESS:
            isAdjusting_ = false; // Stop adjusting if user switches item
            selectedIndex_ = 1 - selectedIndex_;
            animation_.setTargets(selectedIndex_, 2);
            break;
        case InputEvent::BTN_BACK_PRESS: 
            EventDispatcher::getInstance().publish(NavigateBackEvent());
            break;
        default: 
            break;
    }
}

void BrightnessMenu::changeBrightness(App* app, int delta) {
    auto& settings = app->getConfigManager().getSettings();
    
    uint8_t* brightnessValue = (selectedIndex_ == 0) 
                             ? &settings.mainDisplayBrightness 
                             : &settings.auxDisplayBrightness;
    
    int newBrightness = *brightnessValue + delta;

    // Clamp the value between 0 and 255
    if (newBrightness < 0) newBrightness = 0;
    if (newBrightness > 255) newBrightness = 255;
    
    if (*brightnessValue != (uint8_t)newBrightness) {
        *brightnessValue = (uint8_t)newBrightness;
        
        // Apply the setting directly to the correct hardware
        if (selectedIndex_ == 0) {
            app->getHardwareManager().setMainBrightness(*brightnessValue);
        } else {
            app->getHardwareManager().setAuxBrightness(*brightnessValue);
        }

        // Flag that settings are dirty and need to be saved on exit
        app->getConfigManager().requestSave();
    }
}


void BrightnessMenu::draw(App* app, U8G2& display) {
    const int list_start_y = STATUS_BAR_H + 1;
    const int item_h = 18;

    display.setFont(u8g2_font_6x10_tf);

    auto& settings = app->getConfigManager().getSettings();
    uint8_t brightnessValues[] = { settings.mainDisplayBrightness, settings.auxDisplayBrightness };
    const char* labels[] = { "MAIN", "AUX" };

    for (int i = 0; i < 2; ++i) {
        int item_center_y_rel = (int)animation_.itemOffsetY[i];
        float scale = animation_.itemScale[i];
        if (scale <= 0.01f) continue;
        
        int item_center_y_abs = (list_start_y + (63 - list_start_y) / 2) + item_center_y_rel;
        int item_top_y = item_center_y_abs - item_h / 2;

        if (item_top_y > 63 || item_top_y + item_h < list_start_y) continue;
        
        bool isSelected = (i == selectedIndex_);

        if (isSelected) {
            drawRndBox(display, 2, item_top_y, 124, item_h, 2, true);
            display.setDrawColor(0);
        } else {
            display.setDrawColor(1);
        }
        
        // Draw the left-aligned label
        display.drawStr(8, item_center_y_abs + 4, labels[i]);

        // --- THIS IS THE FIX: Use the correct value from the array for each item ---
        int percent = map(brightnessValues[i], 0, 255, 0, 100);
        char valueText[16];
        snprintf(valueText, sizeof(valueText), "< %d%% >", percent);
        int text_w = display.getStrWidth(valueText);
        display.drawStr(128 - text_w - 8, item_center_y_abs + 4, valueText);
    }
    
    display.setDrawColor(1);
}