#include "Event.h"
#include "EventDispatcher.h"
#include "BeaconSpamActiveMenu.h"
#include "App.h"
#include "BeaconSpammer.h"
#include "Config.h" // Needed for SD_ROOT path constants

BeaconSpamActiveMenu::BeaconSpamActiveMenu() : modeToStart_(BeaconSsidMode::RANDOM), initialFrameDrawn_(false) {}

void BeaconSpamActiveMenu::setAttackParameters(BeaconSsidMode mode, const std::string& filePath) {
    modeToStart_ = mode;
    filePathToUse_ = filePath;
}

void BeaconSpamActiveMenu::onEnter(App* app, bool isForwardNav) {
    EventDispatcher::getInstance().subscribe(EventType::APP_INPUT, this);
    initialFrameDrawn_ = false;

    auto& spammer = app->getBeaconSpammer();
    auto rfLock = app->getHardwareManager().requestRfControl(RfClient::WIFI_PROMISCUOUS);
    
    if (!spammer.start(std::move(rfLock), modeToStart_, filePathToUse_)) {
        app->showPopUp("Error", "Failed to acquire RF lock.", [](App* app_cb){
            EventDispatcher::getInstance().publish(NavigateBackEvent());
        }, "OK", "", false);
    }
}

void BeaconSpamActiveMenu::onExit(App* app) {
    EventDispatcher::getInstance().unsubscribe(EventType::APP_INPUT, this);
    app->getHardwareManager().setUiRenderingPaused(false);
    app->getBeaconSpammer().stop();
}

void BeaconSpamActiveMenu::onUpdate(App* app) {
    if (initialFrameDrawn_ && !app->getHardwareManager().isUiRenderingPaused()) {
        app->getHardwareManager().setUiRenderingPaused(true);
    }
}

void BeaconSpamActiveMenu::handleInput(InputEvent event, App* app) {
    switch (event) {
        case InputEvent::BTN_BACK_PRESS:
            EventDispatcher::getInstance().publish(NavigateBackEvent());
            break;
        default:
            break;
    }
}

void BeaconSpamActiveMenu::draw(App* app, U8G2& display) {
    auto& spammer = app->getBeaconSpammer();
    if (!spammer.isActive()) {
        const char* startingMsg = "Starting Attack...";
        display.setFont(u8g2_font_7x13B_tr);
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(startingMsg))/2, 38, startingMsg);
    } else {
        const char* titleMsg = "Beacon Spam Active";
        display.setFont(u8g2_font_7x13B_tr);
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(titleMsg)) / 2, 28, titleMsg);

        // Determine the subtitle based on the attack mode
        const char* modeSubtitle = "Unknown Mode";
        switch (modeToStart_) {
            case BeaconSsidMode::RANDOM:
                modeSubtitle = "Mode: Random";
                break;
            case BeaconSsidMode::FILE_BASED:
                // Differentiate between the probe list and a generic user file
                if (filePathToUse_ == SD_ROOT::DATA_PROBES_SSID_CUMULATIVE) {
                    modeSubtitle = "Mode: From Probes";
                } else {
                    modeSubtitle = "Mode: SD File";
                }
                break;
        }
        
        // Draw the informative subtitle
        display.setFont(u8g2_font_6x10_tf);
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(modeSubtitle)) / 2, 44, modeSubtitle);

        const char* instruction = "Press BACK to Stop";
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(instruction)) / 2, 60, instruction);
    }
    
    initialFrameDrawn_ = true;
}