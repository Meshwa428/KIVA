#include "AirMouseActiveMenu.h"
#include "App.h"
#include "UI_Utils.h"
#include "Event.h"
#include "EventDispatcher.h"
#include "HardwareManager.h"
#include <algorithm> // for std::max/min

AirMouseActiveMenu::AirMouseActiveMenu() 
    : modeToStart_(AirMouseService::Mode::USB), hapticFeedbackUntil_(0),
      uiIsPaused_(false), wasConnected_(false) // Initialize new state
{}

void AirMouseActiveMenu::setMode(AirMouseService::Mode mode) {
    modeToStart_ = mode;
}

void AirMouseActiveMenu::onEnter(App* app, bool isForwardNav) {
    EventDispatcher::getInstance().subscribe(EventType::APP_INPUT, this);
    
    // Reset performance mode state on every entry
    uiIsPaused_ = false;
    wasConnected_ = false;

    if (isForwardNav) {
        if (!app->getAirMouseService().start(modeToStart_)) {
            app->showPopUp("Error", "MPU or HID failed to start.", [](App* app_cb){
                EventDispatcher::getInstance().publish(NavigateBackEvent());
            }, "OK", "", false);
            return;
        }
    }
}

void AirMouseActiveMenu::onExit(App* app) {
    EventDispatcher::getInstance().unsubscribe(EventType::APP_INPUT, this);
    
    // CRITICAL: Always un-pause UI rendering when leaving this menu.
    app->getHardwareManager().setUiRenderingPaused(false);

    // Stop the service only if it's still active.
    if (app->getAirMouseService().isActive()) {
        app->getAirMouseService().stop();
    }
    
}

void AirMouseActiveMenu::onUpdate(App* app) {
    // This function now only handles haptic feedback and performance mode logic.
    if (hapticFeedbackUntil_ > 0 && millis() > hapticFeedbackUntil_) {
        app->getHardwareManager().setVibration(false);
        hapticFeedbackUntil_ = 0;
    }

    // --- NEW PERFORMANCE MODE LOGIC ---
    bool isConnected = app->getAirMouseService().isConnected();

    // Check for a change in connection status
    if (isConnected != wasConnected_) {
        // A change occurred! We need to update the screen.
        wasConnected_ = isConnected;

        // Un-pause rendering to allow the next draw() call to run.
        if (uiIsPaused_) {
            app->getHardwareManager().setUiRenderingPaused(false);
            uiIsPaused_ = false;
        }
        
        // Request a redraw to display the new status.
        app->requestRedraw();
    }
}

uint32_t AirMouseActiveMenu::getResourceRequirements() const {
    return (uint32_t)ResourceRequirement::HOST_PERIPHERAL;
}

void AirMouseActiveMenu::handleInput(InputEvent event, App* app) {
    auto& airMouse = app->getAirMouseService();
    if (!airMouse.isActive()) return;

    switch(event) {
        case InputEvent::BTN_ENCODER_PRESS:
        case InputEvent::BTN_OK_PRESS:
            app->getHardwareManager().setVibration(true);
            hapticFeedbackUntil_ = millis() + 50;
            airMouse.processClick(true);
            break;
        case InputEvent::ENCODER_CW:
            airMouse.processScroll(-1);
            break;
        case InputEvent::ENCODER_CCW:
            airMouse.processScroll(1);
            break;
        case InputEvent::BTN_BACK_PRESS:
            // Un-pause rendering BEFORE stopping the service and navigating away.
            app->getHardwareManager().setUiRenderingPaused(false);
            app->getAirMouseService().stop();
            EventDispatcher::getInstance().publish(NavigateBackEvent());
            break;
        default:
            break;
    }
}

void AirMouseActiveMenu::draw(App* app, U8G2& display) {
    auto& airMouse = app->getAirMouseService();
    
    // This draw function now only renders static status screens.
    if (!airMouse.isConnected()) {
        const char* msg = "Waiting for Host...";
        display.setFont(u8g2_font_7x13B_tr);
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(msg)) / 2, 38, msg);
    } else {
        // Draw the "Active" screen.
        const char* titleMsg = "Air Mouse Active";
        display.setFont(u8g2_font_7x13B_tr);
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(titleMsg)) / 2, 28, titleMsg);

        display.setFont(u8g2_font_6x10_tf);
        const char* instruction = "Press BACK to Exit";
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(instruction)) / 2, 52, instruction);

        // --- PAUSE RENDERING AFTER THIS FRAME ---
        // If we just drew the "Active" screen and the UI isn't already paused, pause it now.
        if (!uiIsPaused_) {
            app->getHardwareManager().setUiRenderingPaused(true);
            uiIsPaused_ = true;
        }
    }
}