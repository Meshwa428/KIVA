#include "AssociationSleepActiveMenu.h"
#include "App.h"
#include "AssociationSleeper.h"
#include "Event.h"
#include "EventDispatcher.h"
#include "UI_Utils.h"

AssociationSleepActiveMenu::AssociationSleepActiveMenu() : initialFrameDrawn_(false) {}

void AssociationSleepActiveMenu::onEnter(App* app, bool isForwardNav) {
    EventDispatcher::getInstance().subscribe(EventType::APP_INPUT, this);
    initialFrameDrawn_ = false; 
    // The attack is assumed to have been started before navigating to this menu.
}

void AssociationSleepActiveMenu::onExit(App* app) {
    EventDispatcher::getInstance().unsubscribe(EventType::APP_INPUT, this);
    // Always ensure the UI is unpaused when leaving this screen.
    app->getHardwareManager().setUiRenderingPaused(false);
    app->getAssociationSleeper().stop();
}

void AssociationSleepActiveMenu::onUpdate(App* app) {
    auto& sleeper = app->getAssociationSleeper();
    
    // If the attack stops for any reason, navigate back.
    if (!sleeper.isActive()) {
        EventDispatcher::getInstance().publish(ReturnToMenuEvent(MenuType::WIFI_ATTACKS_LIST));
        return;
    }

    // --- NEW INTELLIGENT PAUSE LOGIC ---
    bool shouldPauseUi = false;
    switch(sleeper.getAttackType()) {
        case AssociationSleeper::AttackType::PINPOINT_CLIENT:
            // This is a static attack screen, so we can pause rendering after the first frame.
            shouldPauseUi = true;
            break;
        case AssociationSleeper::AttackType::NORMAL:
            // Only pause if we are in the attacking phase, not the initial sniffing phase.
            if (!sleeper.isSniffing()) {
                shouldPauseUi = true;
            }
            break;
        case AssociationSleeper::AttackType::BROADCAST:
            // This screen needs to update the client count, so we should never pause.
            shouldPauseUi = false;
            break;
    }

    // Now, apply the decision.
    if (shouldPauseUi) {
        // If we should be paused, check if the first frame has been drawn.
        // If it has, and the UI isn't paused yet, pause it.
        if (initialFrameDrawn_ && !app->getHardwareManager().isUiRenderingPaused()) {
            app->getHardwareManager().setUiRenderingPaused(true);
        }
    } else {
        // If we should NOT be paused, ensure we are not. This is important
        // for the NORMAL mode when transitioning from sniffing to attacking and back.
        if (app->getHardwareManager().isUiRenderingPaused()) {
            app->getHardwareManager().setUiRenderingPaused(false);
        }
        // Since we are not paused, we need to request redraws to see live updates.
        app->requestRedraw();
    }
}

void AssociationSleepActiveMenu::handleInput(InputEvent event, App* app) {
    if (event == InputEvent::BTN_BACK_PRESS || event == InputEvent::BTN_OK_PRESS) {
        // onExit handles stopping the attack and un-pausing the UI.
        EventDispatcher::getInstance().publish(ReturnToMenuEvent(MenuType::WIFI_ATTACKS_LIST));
    }
}

void AssociationSleepActiveMenu::draw(App* app, U8G2& display) {
    auto& sleeper = app->getAssociationSleeper();
    if (!sleeper.isActive()) {
        const char* msg = "Initializing...";
        display.setFont(u8g2_font_7x13B_tr);
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(msg)) / 2, 38, msg);
        initialFrameDrawn_ = true; 
        return;
    }

    const char* titleMsg = "Association Sleep";
    display.setFont(u8g2_font_7x13B_tr);
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(titleMsg)) / 2, 22, titleMsg);

    display.setFont(u8g2_font_6x10_tf);

    switch(sleeper.getAttackType()) {
        case AssociationSleeper::AttackType::NORMAL:
            {
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
            }
            break;
        case AssociationSleeper::AttackType::BROADCAST:
            {
                const char* modeStr = "Mode: Broadcast";
                display.drawStr((display.getDisplayWidth() - display.getStrWidth(modeStr)) / 2, 36, modeStr);
                char statusStr[32];
                snprintf(statusStr, sizeof(statusStr), "Clients Attacked: %d", sleeper.getClientCount());
                display.drawStr((display.getDisplayWidth() - display.getStrWidth(statusStr)) / 2, 48, statusStr);
            }
            break;
        case AssociationSleeper::AttackType::PINPOINT_CLIENT:
            {
                const char* modeStr = "Mode: Pinpoint";
                display.drawStr((display.getDisplayWidth() - display.getStrWidth(modeStr)) / 2, 36, modeStr);
                char statusStr[32];
                snprintf(statusStr, sizeof(statusStr), "Attacking 1 client");
                display.drawStr((display.getDisplayWidth() - display.getStrWidth(statusStr)) / 2, 48, statusStr);
            }
            break;
    }

    display.setFont(u8g2_font_5x8_t_cyrillic);
    const char* instruction = "Press BACK to Stop";
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(instruction)) / 2, 60, instruction);
    
    // Signal that the first frame has been drawn. This is safe to do every frame.
    initialFrameDrawn_ = true;
}