#include "AssociationSleepActiveMenu.h"
#include "App.h"
#include "Event.h"
#include "EventDispatcher.h"
#include "UI_Utils.h"

AssociationSleepActiveMenu::AssociationSleepActiveMenu() {}

void AssociationSleepActiveMenu::onEnter(App* app, bool isForwardNav) {
    EventDispatcher::getInstance().subscribe(EventType::APP_INPUT, this);
}

void AssociationSleepActiveMenu::onExit(App* app) {
    EventDispatcher::getInstance().unsubscribe(EventType::APP_INPUT, this);
    app->getAssociationSleeper().stop();
}

void AssociationSleepActiveMenu::onUpdate(App* app) {
}

void AssociationSleepActiveMenu::handleInput(InputEvent event, App* app) {
    if (event == InputEvent::BTN_BACK_PRESS || event == InputEvent::BTN_OK_PRESS) {
        EventDispatcher::getInstance().publish(ReturnToMenuEvent(MenuType::WIFI_ATTACKS_LIST));
    }
}

void AssociationSleepActiveMenu::draw(App* app, U8G2& display) {
    auto& sleeper = app->getAssociationSleeper();
    if (!sleeper.isActive()) {
        const char* msg = "Initializing...";
        display.setFont(u8g2_font_7x13B_tr);
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(msg)) / 2, 38, msg);
        return;
    }

    const char* titleMsg = "Association Sleep";
    display.setFont(u8g2_font_7x13B_tr);
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(titleMsg)) / 2, 22, titleMsg);

    display.setFont(u8g2_font_6x10_tf);

    // --- NEW: UI Logic for different modes ---
    if (sleeper.getAttackMode() == AssociationSleeper::AttackMode::TARGETED) {
        std::string targetSsid = "Target: " + sleeper.getTargetSsid();
        char* truncated = truncateText(targetSsid.c_str(), 124, display);
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(truncated)) / 2, 36, truncated);

        char statusStr[32];
        if (sleeper.isSniffing()) {
            snprintf(statusStr, sizeof(statusStr), "Sniffing for clients...");
        } else {
            snprintf(statusStr, sizeof(statusStr), "Attacking %d clients", sleeper.getClientCount());
        }
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(statusStr)) / 2, 48, statusStr);
    } else { // BROADCAST MODE
        const char* modeStr = "Mode: Broadcast";
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(modeStr)) / 2, 36, modeStr);

        char statusStr[32];
        snprintf(statusStr, sizeof(statusStr), "Clients Attacked: %d", sleeper.getClientCount());
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(statusStr)) / 2, 48, statusStr);
    }

    const char* instruction = "Press BACK to Stop";
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(instruction)) / 2, 60, instruction);
}