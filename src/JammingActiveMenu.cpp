#include "JammingActiveMenu.h"
#include "App.h" // For App context to stop jammer, etc.

JammingActiveMenu::JammingActiveMenu() : modeToStart_(JammingMode::IDLE) {}

void JammingActiveMenu::setJammingModeToStart(JammingMode mode) {
    modeToStart_ = mode;
}

void JammingActiveMenu::setJammingConfig(const JammerConfig& config) {
    startConfig_ = config;
}

void JammingActiveMenu::onEnter(App* app) {
    if (modeToStart_ != JammingMode::IDLE) {
        app->getHardwareManager().setPerformanceMode(true);

        // --- NEW RAII-BASED STARTUP ---
        // 1. Request the RF hardware lock from the single source of truth.
        auto rfLock = app->getHardwareManager().requestRfControl(RfClient::NRF_JAMMER);

        // 2. Pass the lock (or its failure) to the jammer to start.
        if (app->getJammer().start(std::move(rfLock), modeToStart_, startConfig_)) {
            // Success, the menu will just display the active state.
        } else {
            // If starting failed, immediately go back and show an error.
            app->getHardwareManager().setPerformanceMode(false);
            app->changeMenu(MenuType::BACK);
            app->showPopUp("Error", "Failed to start jammer.", nullptr, "OK", "", true);
        }
    }
}

void JammingActiveMenu::onUpdate(App* app) {
    // The main App::loop() throttles updates, so this might not be called often.
}

void JammingActiveMenu::onExit(App* app) {
    // This now correctly triggers the RAII cleanup.
    if (app->getJammer().isActive()) {
        app->getJammer().stop();
    }
    app->getHardwareManager().setPerformanceMode(false);
    
    // Reset state for the next time we enter.
    modeToStart_ = JammingMode::IDLE;
    startConfig_.customChannels.clear();
}

void JammingActiveMenu::handleInput(App* app, InputEvent event) {
    // Only care about the back button to stop the process.
    if (event == InputEvent::BTN_BACK_PRESS || event == InputEvent::BTN_OK_PRESS || event == InputEvent::BTN_ENCODER_PRESS) {
        // onExit will handle the actual stopping.
        app->changeMenu(MenuType::BACK);
    }
}

void JammingActiveMenu::draw(App* app, U8G2& display) {
    // --- SIMPLIFIED DRAWING LOGIC ---
    auto& jammer = app->getJammer();
     if (!jammer.isActive()) {
        const char* startingMsg = "Starting Jammer...";
        display.setFont(u8g2_font_7x13B_tr);
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(startingMsg))/2, 38, startingMsg);
        return;
    }

    const char* modeText = jammer.getModeString();
    
    display.setFont(u8g2_font_7x13B_tr);
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(modeText))/2, 28, modeText);

    display.setFont(u8g2_font_6x10_tf);
    const char* instruction = "Press BACK to Stop";
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(instruction))/2, 52, instruction);
}