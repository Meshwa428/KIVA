#include "BleSpamActiveMenu.h"
#include "App.h"
#include "Event.h"
#include "EventDispatcher.h"

BleSpamActiveMenu::BleSpamActiveMenu() : modeToStart_(BleSpamMode::ALL) {}

void BleSpamActiveMenu::setSpamModeToStart(BleSpamMode mode) {
    modeToStart_ = mode;
}

void BleSpamActiveMenu::onEnter(App* app, bool isForwardNav) {
    EventDispatcher::getInstance().subscribe(EventType::APP_INPUT, this);
    // REMOVED: app->getHardwareManager().setPerformanceMode(true);
    app->getBleSpammer().start(modeToStart_);
}

void BleSpamActiveMenu::onUpdate(App* app) {
    // BLE Spam needs to update the screen to show the current attack type when in "ALL" mode.
    // Request a redraw to keep the UI live.
    app->requestRedraw();
}

void BleSpamActiveMenu::onExit(App* app) {
    EventDispatcher::getInstance().unsubscribe(EventType::APP_INPUT, this);
    app->getBleSpammer().stop();
    // REMOVED: app->getHardwareManager().setPerformanceMode(false);
}

void BleSpamActiveMenu::handleInput(InputEvent event, App* app) {
    if (event == InputEvent::BTN_BACK_PRESS || event == InputEvent::BTN_OK_PRESS) {
        EventDispatcher::getInstance().publish(NavigateBackEvent());
    }
}

void BleSpamActiveMenu::draw(App* app, U8G2& display) {
    auto& spammer = app->getBleSpammer();
    
    const char* modeText = spammer.getModeString();
    
    display.setFont(u8g2_font_7x13B_tr);
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(modeText))/2, 28, modeText);

    display.setFont(u8g2_font_6x10_tf);
    const char* instruction = "Press BACK to Stop";
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(instruction)) / 2, 52, instruction);
}