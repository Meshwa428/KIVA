#include "HandshakeCaptureActiveMenu.h"
#include "App.h"
#include "HandshakeCapture.h"
#include "UI_Utils.h"

HandshakeCaptureActiveMenu::HandshakeCaptureActiveMenu() : startTime_(0) {}

void HandshakeCaptureActiveMenu::onEnter(App* app, bool isForwardNav) {
    HandshakeCapture& capture = app->getHandshakeCapture();
    if (capture.getConfig().type == HandshakeCaptureType::SCANNER) {
        capture.startScanner();
    }
    // For targeted, the start is called after a network is selected from the list.
    startTime_ = millis();
}

void HandshakeCaptureActiveMenu::onExit(App* app) {
    app->getHandshakeCapture().stop();
}

void HandshakeCaptureActiveMenu::onUpdate(App* app) {
    // Nothing to do here, the capture runs in the background
}

void HandshakeCaptureActiveMenu::handleInput(App* app, InputEvent event) {
    if (event == InputEvent::BTN_BACK_PRESS) {
        app->returnToMenu(MenuType::HANDSHAKE_CAPTURE_MENU);
    }
}

void HandshakeCaptureActiveMenu::draw(App* app, U8G2& display) {
    HandshakeCapture& capture = app->getHandshakeCapture();
    const HandshakeCaptureConfig& config = capture.getConfig();

    char buffer[64];

    // Title
    const char* title = (config.mode == HandshakeCaptureMode::EAPOL) ? "EAPOL Sniffer" : "PMKID Sniffer";
    int titleWidth = display.getStrWidth(title);
    display.drawStr((128 - titleWidth) / 2, 16, title);

    // Mode
    const char* type_str = (config.type == HandshakeCaptureType::SCANNER) ? "Scanner" : "Targeted";
    snprintf(buffer, sizeof(buffer), "Mode: %s", type_str);
    display.drawStr(5, 30, buffer);

    // Stats
    snprintf(buffer, sizeof(buffer), "Packets: %u", capture.getPacketCount());
    display.drawStr(5, 42, buffer);

    if (config.mode == HandshakeCaptureMode::EAPOL) {
        snprintf(buffer, sizeof(buffer), "Handshakes: %d", capture.getHandshakeCount());
        display.drawStr(5, 54, buffer);
    } else {
        snprintf(buffer, sizeof(buffer), "PMKIDs: %d", capture.getPmkidCount());
        display.drawStr(5, 54, buffer);
    }

    // Runtime
    unsigned long runtime = (millis() - startTime_) / 1000;
    snprintf(buffer, sizeof(buffer), "Time: %lus", runtime);
    int runtimeWidth = display.getStrWidth(buffer);
    display.drawStr(128 - runtimeWidth - 5, 30, buffer);
}
