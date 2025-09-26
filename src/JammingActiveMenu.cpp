#include "Event.h"
#include "EventDispatcher.h"
#include "JammingActiveMenu.h"
#include "App.h" // For App context to stop jammer, etc.

JammingActiveMenu::JammingActiveMenu() : modeToStart_(JammingMode::IDLE) {}

void JammingActiveMenu::setJammingModeToStart(JammingMode mode) {
    modeToStart_ = mode;
}

void JammingActiveMenu::setJammingConfig(const JammerConfig& config) {
    startConfig_ = config;
}

void JammingActiveMenu::onEnter(App* app, bool isForwardNav) {
    EventDispatcher::getInstance().subscribe(EventType::APP_INPUT, this);
    initialFrameDrawn_ = false; // Reset the flag on entry

    if (modeToStart_ != JammingMode::IDLE) {
        // We do NOT pause the UI here yet.
        auto rfLock = app->getHardwareManager().requestRfControl(RfClient::NRF_JAMMER);
        if (!app->getJammer().start(std::move(rfLock), modeToStart_, startConfig_)) {
            EventDispatcher::getInstance().publish(NavigateBackEvent());
            app->showPopUp("Error", "Failed to start jammer.", nullptr, "OK", "", true);
        }
    }
}

void JammingActiveMenu::onUpdate(App* app) {
    // This logic ensures the pause happens on the frame AFTER the first draw.
    if (initialFrameDrawn_ && !app->getHardwareManager().isUiRenderingPaused()) {
        app->getHardwareManager().setUiRenderingPaused(true);
    }
}

void JammingActiveMenu::onExit(App* app) {
    EventDispatcher::getInstance().unsubscribe(EventType::APP_INPUT, this);
    
    // Un-pause the UI FIRST to ensure the next menu renders correctly.
    app->getHardwareManager().setUiRenderingPaused(false);

    if (app->getJammer().isActive()) {
        app->getJammer().stop();
    }
    
    modeToStart_ = JammingMode::IDLE;
    startConfig_ = JammerConfig();
}

void JammingActiveMenu::handleInput(InputEvent event, App* app) {
    // Only care about the back button to stop the process.
    if (event == InputEvent::BTN_BACK_PRESS || event == InputEvent::BTN_OK_PRESS || event == InputEvent::BTN_ENCODER_PRESS) {
        // onExit will handle the actual stopping.
        EventDispatcher::getInstance().publish(NavigateBackEvent());
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
    } else {
        const char* modeText = jammer.getModeString();
        display.setFont(u8g2_font_7x13B_tr);
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(modeText))/2, 28, modeText);
        display.setFont(u8g2_font_6x10_tf);
        const char* instruction = "Press BACK to Stop";
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(instruction))/2, 52, instruction);
    }
    
    // Signal that the first frame has been drawn.
    initialFrameDrawn_ = true;
}