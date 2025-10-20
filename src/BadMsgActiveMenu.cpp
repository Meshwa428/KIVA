#include "BadMsgActiveMenu.h"
#include "App.h"
#include "BadMsgAttacker.h"
#include "Event.h"
#include "EventDispatcher.h"
#include "UI_Utils.h"

BadMsgActiveMenu::BadMsgActiveMenu() {}

void BadMsgActiveMenu::onEnter(App* app, bool isForwardNav) {
    EventDispatcher::getInstance().subscribe(EventType::APP_INPUT, this);
}

void BadMsgActiveMenu::onExit(App* app) {
    EventDispatcher::getInstance().unsubscribe(EventType::APP_INPUT, this);
    app->getBadMsgAttacker().stop();
}

void BadMsgActiveMenu::onUpdate(App* app) {
    app->requestRedraw(); // Keep screen live to show packet count
}

void BadMsgActiveMenu::handleInput(InputEvent event, App* app) {
    if (event == InputEvent::BTN_BACK_PRESS || event == InputEvent::BTN_OK_PRESS) {
        EventDispatcher::getInstance().publish(ReturnToMenuEvent(MenuType::WIFI_ATTACKS_LIST));
    }
}

void BadMsgActiveMenu::draw(App* app, U8G2& display) {
    auto& attacker = app->getBadMsgAttacker();
    if (!attacker.isActive()) {
        const char* msg = "Initializing...";
        display.setFont(u8g2_font_7x13B_tr);
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(msg)) / 2, 38, msg);
        return;
    }

    const char* titleMsg = "Bad Msg Attack";
    display.setFont(u8g2_font_7x13B_tr);
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(titleMsg)) / 2, 22, titleMsg);
    
    display.setFont(u8g2_font_6x10_tf);

    std::string targetSsid = "Target: " + std::string(attacker.getConfig().targetAp.ssid);
    char* truncated = truncateText(targetSsid.c_str(), 124, display);
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(truncated)) / 2, 36, truncated);

    char statusStr[32];
    if (attacker.getConfig().type == BadMsgAttacker::AttackType::TARGET_AP) {
        if (attacker.isSniffing()) {
            snprintf(statusStr, sizeof(statusStr), "Sniffing clients...");
        } else {
            snprintf(statusStr, sizeof(statusStr), "Attacking %d clients", attacker.getClientCount());
        }
    } else {
        snprintf(statusStr, sizeof(statusStr), "Attacking 1 client");
    }
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(statusStr)) / 2, 48, statusStr);

    display.setFont(u8g2_font_5x8_t_cyrillic);
    const char* instruction = "Press BACK to Stop";
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(instruction)) / 2, 60, instruction);
}
