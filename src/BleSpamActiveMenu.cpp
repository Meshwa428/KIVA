#include "BleSpamActiveMenu.h"
#include "App.h"
#include "Logger.h"

BleSpamActiveMenu::BleSpamActiveMenu() : spamType_(BleSpam::AppleJuice), spamAll_(false) {}

void BleSpamActiveMenu::setSpamType(BleSpam::EBLEPayloadType type) {
    spamType_ = type;
}

void BleSpamActiveMenu::setSpamAll(bool all) {
    spamAll_ = all;
}

void BleSpamActiveMenu::onEnter(App* app, bool isForwardNav) {
    app->getHardwareManager().setPerformanceMode(true);
    if (spamAll_) {
        app->getBleSpammer().startAll();
        LOG(LogLevel::INFO, "BLE_SPAM", "Started spamming ALL types.");
    } else {
        app->getBleSpammer().start(spamType_);
        LOG(LogLevel::INFO, "BLE_SPAM", "Started spamming type: %s", getSpamTypeString());
    }
}

void BleSpamActiveMenu::onUpdate(App* app) {
    // The library handles its own background task
}

void BleSpamActiveMenu::onExit(App* app) {
    app->getBleSpammer().stop();
    app->getHardwareManager().setPerformanceMode(false);
    LOG(LogLevel::INFO, "BLE_SPAM", "Stopped spamming.");
}

void BleSpamActiveMenu::handleInput(App* app, InputEvent event) {
    if (event == InputEvent::BTN_BACK_PRESS || event == InputEvent::BTN_OK_PRESS) {
        app->changeMenu(MenuType::BACK);
    }
}

const char* BleSpamActiveMenu::getSpamTypeString() const {
    if (spamAll_) return "All Types";
    switch (spamType_) {
        case BleSpam::AppleJuice: return "AppleJuice";
        case BleSpam::SourApple: return "Sour Apple";
        case BleSpam::Samsung: return "Samsung";
        case BleSpam::Google: return "Google";
        case BleSpam::Microsoft: return "Microsoft";
        default: return "Unknown";
    }
}

void BleSpamActiveMenu::draw(App* app, U8G2& display) {
    const char* modeText = getSpamTypeString();
    
    display.setFont(u8g2_font_7x13B_tr);
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(modeText))/2, 28, modeText);

    display.setFont(u8g2_font_6x10_tf);
    const char* instruction = "Press BACK to Stop";
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(instruction)) / 2, 52, instruction);
}