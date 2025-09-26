
#include "Event.h"
#include "EventDispatcher.h"
#include "HandshakeCaptureActiveMenu.h"
#include "App.h"
#include "HandshakeCapture.h"
#include "UI_Utils.h"

HandshakeCaptureActiveMenu::HandshakeCaptureActiveMenu() : startTime_(0) {}

void HandshakeCaptureActiveMenu::onEnter(App* app, bool isForwardNav) {
    EventDispatcher::getInstance().subscribe(EventType::APP_INPUT, this);
    HandshakeCapture& capture = app->getHandshakeCapture();
    if (capture.getConfig().type == HandshakeCaptureType::SCANNER) {
        capture.startScanner();
    }
    startTime_ = millis();
}

void HandshakeCaptureActiveMenu::onExit(App* app) {
    EventDispatcher::getInstance().unsubscribe(EventType::APP_INPUT, this);
    app->getHandshakeCapture().stop();
}

void HandshakeCaptureActiveMenu::onUpdate(App* app) {
    // --- ADD THIS LINE ---
    // Since the stats (packet count, timer) are always changing,
    // we need to request a redraw on every update loop.
    app->requestRedraw();
}

void HandshakeCaptureActiveMenu::handleInput(InputEvent event, App* app) {
    if (event == InputEvent::BTN_BACK_PRESS) {
        EventDispatcher::getInstance().publish(ReturnToMenuEvent(MenuType::HANDSHAKE_CAPTURE_MENU));
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

    // Status Section
    display.setFont(u8g2_font_6x10_tf);
    if (config.type == HandshakeCaptureType::SCANNER) {
        snprintf(buffer, sizeof(buffer), "Mode: Scanning");
    } else {
        const char* stateStr = "Unknown";
        switch(capture.getTargetedState()) {
            case TargetedAttackState::WAITING_FOR_DEAUTH: stateStr = "Deauthing..."; break;
            case TargetedAttackState::LISTENING:          stateStr = "Listening..."; break;
            case TargetedAttackState::COOLDOWN:           stateStr = "Cooldown...";  break;
        }
        snprintf(buffer, sizeof(buffer), "Status: %s", stateStr);
    }
    display.drawStr(5, 30, buffer);

    // Stats Section
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
