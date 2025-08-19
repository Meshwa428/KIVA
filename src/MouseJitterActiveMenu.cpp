#include "MouseJitterActiveMenu.h"
#include "App.h"
#include "UI_Utils.h"

MouseJitterActiveMenu::MouseJitterActiveMenu() : mode_(JitterMode::USB), activeMouse_(nullptr) {}

void MouseJitterActiveMenu::setJitterMode(JitterMode mode) {
    mode_ = mode;
}

void MouseJitterActiveMenu::onEnter(App* app, bool isForwardNav) {
    app->getHardwareManager().setPerformanceMode(true);

    if (mode_ == JitterMode::USB) {
        activeMouse_ = &app->getUsbMouse();
        activeMouse_->begin();
        USB.begin(); // Start the global USB stack
    } else { // BLE
        activeMouse_ = &app->getBleMouse();
        activeMouse_->begin(); // This starts advertising
    }
}

void MouseJitterActiveMenu::onUpdate(App* app) {
    if (activeMouse_ && activeMouse_->isConnected()) {
        int8_t dx = (esp_random() % 21) - 10; // Random value between -10 and 10
        int8_t dy = (esp_random() % 21) - 10;
        activeMouse_->move(dx, dy);
        delay(30); // A small delay to control the jitter speed
    }
}

void MouseJitterActiveMenu::onExit(App* app) {
    if (activeMouse_) {
        activeMouse_->end();
    }
    if (mode_ == JitterMode::USB) {
        // USB.end();
        // but USB doesn't have end method
        // TODO: Find a way to end USB stack
    }
    activeMouse_ = nullptr;
    app->getHardwareManager().setPerformanceMode(false);
}

void MouseJitterActiveMenu::handleInput(App* app, InputEvent event) {
    if (event != InputEvent::NONE) {
        app->changeMenu(MenuType::BACK);
    }
}

void MouseJitterActiveMenu::draw(App* app, U8G2& display) {
    const char* modeText = (mode_ == JitterMode::USB) ? "USB Mode" : "BLE Mode";
    const char* statusText = (activeMouse_ && activeMouse_->isConnected()) ? "Jittering..." : "Waiting for Host...";

    display.setFont(u8g2_font_7x13B_tr);
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(modeText)) / 2, 28, modeText);

    display.setFont(u8g2_font_6x10_tf);
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(statusText)) / 2, 42, statusText);

    const char* instruction = "Press any key to Stop";
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(instruction)) / 2, 60, instruction);
}