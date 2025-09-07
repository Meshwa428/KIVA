#include "Event.h"
#include "EventDispatcher.h"
#include "BeaconSpamActiveMenu.h"
#include "App.h"
#include "BeaconSpammer.h"

BeaconSpamActiveMenu::BeaconSpamActiveMenu() : modeToStart_(BeaconSsidMode::RANDOM) {}

void BeaconSpamActiveMenu::setAttackParameters(BeaconSsidMode mode, const std::string& filePath) {
    modeToStart_ = mode;
    filePathToUse_ = filePath;
}

void BeaconSpamActiveMenu::onEnter(App* app, bool isForwardNav) {
    EventDispatcher::getInstance().subscribe(EventType::INPUT, this);
    app->getHardwareManager().setPerformanceMode(true);

    auto& spammer = app->getBeaconSpammer();
    auto rfLock = app->getHardwareManager().requestRfControl(RfClient::WIFI_PROMISCUOUS);
    
    // This now only handles a low-level hardware lock failure, which is the correct behavior.
    if (!spammer.start(std::move(rfLock), modeToStart_, filePathToUse_)) {
        app->getHardwareManager().setPerformanceMode(false);
        // This popup will now only show if there's a hardware-level problem.
        app->showPopUp("Error", "Failed to acquire RF lock.", [](App* app_cb){
            app_cb->changeMenu(MenuType::BACK);
        }, "OK", "", false);
    }
}

void BeaconSpamActiveMenu::onExit(App* app) {
    EventDispatcher::getInstance().unsubscribe(EventType::INPUT, this);
    app->getHardwareManager().setPerformanceMode(false);
    app->getBeaconSpammer().stop();
}

void BeaconSpamActiveMenu::onUpdate(App* app) {}

void BeaconSpamActiveMenu::handleInput(InputEvent event, App* app) {
    switch (event) {
        case InputEvent::BTN_BACK_PRESS:
            EventDispatcher::getInstance().publish(Event{EventType::NAVIGATE_BACK});
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
        return;
    }
    
    const char* titleMsg = "Beacon Spam Active";
    display.setFont(u8g2_font_7x13B_tr);
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(titleMsg)) / 2, 28, titleMsg);

    display.setFont(u8g2_font_6x10_tf);
    const char* instruction = "Press BACK to Stop";
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(instruction)) / 2, 52, instruction);
}
