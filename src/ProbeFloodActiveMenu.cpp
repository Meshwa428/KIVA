#include "Event.h"
#include "EventDispatcher.h"
#include "ProbeFloodActiveMenu.h"
#include "App.h"
#include "UI_Utils.h"

ProbeFloodActiveMenu::ProbeFloodActiveMenu() : modeToStart_(ProbeFloodMode::RANDOM) {}

void ProbeFloodActiveMenu::setAttackParameters(ProbeFloodMode mode, const std::string& filePath) {
    modeToStart_ = mode;
    filePathToUse_ = filePath;
    // Clear the target network to ensure this mode is used
    memset(&targetNetwork_, 0, sizeof(WifiNetworkInfo));
}

void ProbeFloodActiveMenu::setAttackParameters(const WifiNetworkInfo& targetNetwork) {
    modeToStart_ = ProbeFloodMode::PINPOINT_AP;
    targetNetwork_ = targetNetwork;
    filePathToUse_ = "";
}

void ProbeFloodActiveMenu::onEnter(App* app, bool isForwardNav) {
    EventDispatcher::getInstance().subscribe(EventType::APP_INPUT, this);
    auto& flooder = app->getProbeFlooder();
    auto rfLock = app->getHardwareManager().requestRfControl(RfClient::WIFI_PROMISCUOUS);
    
    bool success = false;
    if (modeToStart_ == ProbeFloodMode::PINPOINT_AP) {
        success = flooder.start(std::move(rfLock), targetNetwork_);
    } else {
        // This now correctly calls the start overload for RANDOM, FILE_BASED, and both AUTO modes
        success = flooder.start(std::move(rfLock), modeToStart_);
    }

    if (!success) {
        app->showPopUp("Error", "Failed to start attack.", [](App* app_cb){
            EventDispatcher::getInstance().publish(NavigateBackEvent());
        }, "OK", "", false);
    }
}

void ProbeFloodActiveMenu::onExit(App* app) {
    EventDispatcher::getInstance().unsubscribe(EventType::APP_INPUT, this);
    app->getProbeFlooder().stop();
}

void ProbeFloodActiveMenu::onUpdate(App* app) {
    app->requestRedraw();
}

void ProbeFloodActiveMenu::handleInput(InputEvent event, App* app) {
    if (event == InputEvent::BTN_BACK_PRESS || event == InputEvent::BTN_OK_PRESS) {
        EventDispatcher::getInstance().publish(NavigateBackEvent());
    }
}

bool ProbeFloodActiveMenu::drawCustomStatusBar(App* app, U8G2& display) {
    auto& flooder = app->getProbeFlooder();
    
    display.setFont(u8g2_font_6x10_tf);
    display.setDrawColor(1);

    display.drawStr(2, 8, getTitle());

    char channelStr[16];
    snprintf(channelStr, sizeof(channelStr), "CH: %d", flooder.getCurrentChannel());
    
    int textWidth = display.getStrWidth(channelStr);
    display.drawStr(128 - textWidth - 2, 8, channelStr);
    
    display.drawLine(0, STATUS_BAR_H - 1, 127, STATUS_BAR_H - 1);

    return true;
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
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(titleMsg)) / 2, 22, titleMsg);

    display.setFont(u8g2_font_6x10_tf);

    if (flooder.getMode() == ProbeFloodMode::PINPOINT_AP) {
        char targetStr[48];
        snprintf(targetStr, sizeof(targetStr), "Target: %s", flooder.getCurrentTargetSsid().c_str());
        char* truncated = truncateText(targetStr, 124, display);
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(truncated)) / 2, 36, truncated);
    } else if (flooder.getMode() == ProbeFloodMode::AUTO_BROADCAST_PINPOINT || flooder.getMode() == ProbeFloodMode::AUTO_SNIFFED_PINPOINT) {
        if (flooder.getAutoPinpointState() == AutoPinpointState::SCANNING) {
            display.drawStr((display.getDisplayWidth() - display.getStrWidth("Scanning...")) / 2, 36, "Scanning...");
        } else {
            char targetStr[48];
            snprintf(targetStr, sizeof(targetStr), "Target: %s", flooder.getCurrentTargetSsid().c_str());
            char* truncated = truncateText(targetStr, 124, display);
            display.drawStr((display.getDisplayWidth() - display.getStrWidth(truncated)) / 2, 36, truncated);
        }
    }

    char packetStr[32];
    snprintf(packetStr, sizeof(packetStr), "Packets Sent: %u", flooder.getPacketCounter());
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(packetStr)) / 2, 48, packetStr);

    const char* instruction = "Press BACK to Stop";
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(instruction)) / 2, 60, instruction);
}