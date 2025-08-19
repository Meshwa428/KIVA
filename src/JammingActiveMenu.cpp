#include "JammingActiveMenu.h"
#include "App.h"

JammingActiveMenu::JammingActiveMenu() : modeToStart_(JammingMode::IDLE) {}

void JammingActiveMenu::setJammingModeToStart(JammingMode mode) {
    modeToStart_ = mode;
}

void JammingActiveMenu::setJammingConfig(const JammerConfig& config) {
    startConfig_ = config;
}

void JammingActiveMenu::onEnter(App* app, bool isForwardNav) {
    if (modeToStart_ != JammingMode::IDLE) {
        app->getHardwareManager().setPerformanceMode(true);

        if (app->getSweeper().startJammer(modeToStart_, startConfig_)) {
            // Success
        } else {
            // Failure
            app->getHardwareManager().setPerformanceMode(false);
            app->changeMenu(MenuType::BACK);
            app->showPopUp("Error", "Failed to start jammer.", nullptr, "OK", "", true);
        }
    }
}

void JammingActiveMenu::onUpdate(App* app) {
    // The main App::loop() now calls sweeper.handleJammer()
}

void JammingActiveMenu::onExit(App* app) {
    if (app->getSweeper().isJamming()) {
        app->getSweeper().stopJammer();
    }
    app->getHardwareManager().setPerformanceMode(false);
    
    modeToStart_ = JammingMode::IDLE;
    startConfig_ = JammerConfig();
}

void JammingActiveMenu::handleInput(App* app, InputEvent event) {
    if (event == InputEvent::BTN_BACK_PRESS || event == InputEvent::BTN_OK_PRESS || event == InputEvent::BTN_ENCODER_PRESS) {
        app->changeMenu(MenuType::BACK);
    }
}

void JammingActiveMenu::draw(App* app, U8G2& display) {
    auto& sweeper = app->getSweeper();
    if (!sweeper.isJamming()) {
        const char* startingMsg = "Starting Jammer...";
        display.setFont(u8g2_font_7x13B_tr);
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(startingMsg))/2, 38, startingMsg);
        return;
    }

    const char* modeText = sweeper.getModeString();
    
    display.setFont(u8g2_font_7x13B_tr);
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(modeText))/2, 28, modeText);

    display.setFont(u8g2_font_6x10_tf);
    const char* instruction = "Press BACK to Stop";
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(instruction))/2, 52, instruction);
}