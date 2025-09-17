#include "Event.h"
#include "EventDispatcher.h"
#include "Event.h"
#include "EventDispatcher.h"
#include "ProbeFloodActiveMenu.h"
#include "App.h"

ProbeFloodActiveMenu::ProbeFloodActiveMenu() : modeToStart_(ProbeFloodMode::RANDOM) {}

void ProbeFloodActiveMenu::setAttackParameters(ProbeFloodMode mode, const std::string& filePath) {
    modeToStart_ = mode;
    filePathToUse_ = filePath;
}

void ProbeFloodActiveMenu::onEnter(App* app, bool isForwardNav) {
    EventDispatcher::getInstance().subscribe(EventType::APP_INPUT, this);
    app->getHardwareManager().setPerformanceMode(true);
    auto& flooder = app->getProbeFlooder();
    auto rfLock = app->getHardwareManager().requestRfControl(RfClient::WIFI_PROMISCUOUS);
    
    if (!flooder.start(std::move(rfLock), modeToStart_, filePathToUse_)) {
        app->getHardwareManager().setPerformanceMode(false);
        app->showPopUp("Error", "Failed to start attack.", [](App* app_cb){
            EventDispatcher::getInstance().publish(NavigateBackEvent());
        }, "OK", "", false);
    }
}

void ProbeFloodActiveMenu::onExit(App* app) {
    EventDispatcher::getInstance().unsubscribe(EventType::APP_INPUT, this);
    app->getHardwareManager().setPerformanceMode(false);
    app->getProbeFlooder().stop();
}

void ProbeFloodActiveMenu::onUpdate(App* app) {}

void ProbeFloodActiveMenu::handleInput(InputEvent event, App* app) {
    if (event == InputEvent::BTN_BACK_PRESS || event == InputEvent::BTN_OK_PRESS) {
        EventDispatcher::getInstance().publish(NavigateBackEvent());
    }
}

bool ProbeFloodActiveMenu::drawCustomStatusBar(App* app, U8G2& display) {
    auto& flooder = app->getProbeFlooder();
    
    display.setFont(u8g2_font_6x10_tf);
    display.setDrawColor(1);

    // Left side: Attack Title
    display.drawStr(2, 8, getTitle());

    // Right side: Current Channel
    char channelStr[16];
    snprintf(channelStr, sizeof(channelStr), "CH: %d", flooder.getCurrentChannel());
    int textWidth = display.getStrWidth(channelStr);
    display.drawStr(128 - textWidth - 2, 8, channelStr);
    
    // Bottom line
    display.drawLine(0, STATUS_BAR_H - 1, 127, STATUS_BAR_H - 1);

    return true; // We handled the status bar drawing.
}

void ProbeFloodActiveMenu::draw(App* app, U8G2& display) {
    auto& flooder = app->getProbeFlooder();
    if (!flooder.isActive()) {
        const char* startingMsg = "Starting Attack...";
        display.setFont(u8g2_font_7x13B_tr);
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(startingMsg))/2, 38, startingMsg);
        return;
    }
    
    display.setFont(u8g2_font_7x13B_tr);
    const char* titleMsg = "Probe Flood Active";
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(titleMsg)) / 2, 28, titleMsg);

    display.setFont(u8g2_font_6x10_tf);
    char packetStr[32];
    snprintf(packetStr, sizeof(packetStr), "Packets Sent: %u", flooder.getPacketCounter());
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(packetStr)) / 2, 44, packetStr);

    const char* instruction = "Press BACK to Stop";
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(instruction)) / 2, 60, instruction);
}
