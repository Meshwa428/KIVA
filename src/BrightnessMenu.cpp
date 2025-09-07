#include "Event.h"
#include "EventDispatcher.h"
#include "BrightnessMenu.h"
#include "App.h"
#include "UI_Utils.h"
#include "ConfigManager.h"

BrightnessMenu::BrightnessMenu() : 
    selectedIndex_(0)
{}

void BrightnessMenu::onEnter(App* app, bool isForwardNav) {
    EventDispatcher::getInstance().subscribe(EventType::APP_INPUT, this);
    if (isForwardNav) {
        selectedIndex_ = 0;
    }
    // --- THIS IS THE FIX ---
    // Resize the animation vectors to hold 2 items (Main and Aux).
    animation_.resize(2);
    animation_.init();
    animation_.setTargets(selectedIndex_, 2); // We have 2 items: MAIN and AUX
}

void BrightnessMenu::onUpdate(App* app) {
    animation_.update();
}

void BrightnessMenu::onExit(App* app) {
    EventDispatcher::getInstance().unsubscribe(EventType::APP_INPUT, this);
}

void BrightnessMenu::handleInput(InputEvent event, App* app) {
    // Handle adjustment for the currently selected item
    if (event == InputEvent::BTN_LEFT_PRESS || event == InputEvent::ENCODER_CCW) {
        changeBrightness(app, -13); // Decrease by ~5%
        return;
    }
    if (event == InputEvent::BTN_RIGHT_PRESS || event == InputEvent::ENCODER_CW) {
        changeBrightness(app, 13); // Increase by ~5%
        return;
    }

    // Handle switching between items
    switch(event) {
        case InputEvent::BTN_UP_PRESS:
        case InputEvent::BTN_DOWN_PRESS:
            selectedIndex_ = 1 - selectedIndex_; // Toggle between 0 and 1
            break;
        case InputEvent::BTN_BACK_PRESS: 
            EventDispatcher::getInstance().publish(Event{EventType::NAVIGATE_BACK});
            break;
        default: 
            break;
    }
    animation_.setTargets(selectedIndex_, 2);
}

void BrightnessMenu::changeBrightness(App* app, int delta) {
    auto& settings = app->getConfigManager().getSettings();
    uint8_t* brightnessValue = (selectedIndex_ == 0) 
                             ? &settings.mainDisplayBrightness 
                             : &settings.auxDisplayBrightness;
    
    int newBrightness = *brightnessValue + delta;
    if (newBrightness < 0) newBrightness = 0;
    if (newBrightness > 255) newBrightness = 255;
    *brightnessValue = newBrightness;
    
    app->getConfigManager().saveSettings();
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

        // Draw the right-aligned value selector
        int percent = map(brightnessValues[i], 0, 255, 0, 100);
        char valueText[16];
        snprintf(valueText, sizeof(valueText), "< %d%% >", percent);
        int text_w = display.getStrWidth(valueText);
        display.drawStr(128 - text_w - 8, item_center_y_abs + 4, valueText);
    }
    
    display.setDrawColor(1);
}
