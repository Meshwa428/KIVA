#include "BeaconSpamActiveMenu.h"
#include "App.h"
#include "BeaconSpammer.h"

BeaconSpamActiveMenu::BeaconSpamActiveMenu() : modeToStart_(BeaconSsidMode::RANDOM) {}

void BeaconSpamActiveMenu::setAttackParameters(BeaconSsidMode mode, const std::string& filePath) {
    modeToStart_ = mode;
    filePathToUse_ = filePath;
}

void BeaconSpamActiveMenu::onEnter(App* app) {
    // --- USE THE CORRECT FUNCTION ---
    app->getHardwareManager().setPerformanceMode(true);

    auto& spammer = app->getBeaconSpammer();
    auto rfLock = app->getHardwareManager().requestRfControl(RfClient::WIFI_PROMISCUOUS);
    
    if (!spammer.start(std::move(rfLock), modeToStart_, filePathToUse_)) {
        // If starting failed, revert performance mode
        app->getHardwareManager().setPerformanceMode(false);
        app->changeMenu(MenuType::BACK);
        app->showPopUp("Error", "Failed to start Beacon Spam.", nullptr, "OK", "", true);
    }
}

void BeaconSpamActiveMenu::onExit(App* app) {
    // --- USE THE CORRECT FUNCTION ---
    app->getHardwareManager().setPerformanceMode(false);
    app->getBeaconSpammer().stop();
}

void BeaconSpamActiveMenu::onUpdate(App* app) {}

void BeaconSpamActiveMenu::handleInput(App* app, InputEvent event) {
    if (event == InputEvent::BTN_BACK_PRESS || event == InputEvent::BTN_OK_PRESS) {
        app->changeMenu(MenuType::BACK); // onExit will handle stopping the attack.
    }
}

void BeaconSpamActiveMenu::draw(App* app, U8G2& display) {
    // --- SIMPLIFIED DRAWING LOGIC ---
    auto& spammer = app->getBeaconSpammer();
    if (!spammer.isActive()) {
        // This can happen for a frame or two if starting fails.
        // Draw a "Starting..." message.
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