#include "BadUSBActiveMenu.h"
#include "App.h"
#include "UI_Utils.h"

BadUSBActiveMenu::BadUSBActiveMenu() : entryTime_(0) {}

void BadUSBActiveMenu::onEnter(App* app, bool isForwardNav) {
    entryTime_ = millis();
}

void BadUSBActiveMenu::onExit(App* app) {
    // Ensure script is stopped if user backs out manually
    if(app->getBadUSB().isActive()) {
        app->getBadUSB().stopScript();
    }
}

void BadUSBActiveMenu::onUpdate(App* app) {
    auto& badUsb = app->getBadUSB();
    // Automatically go back to the script list 2 seconds after finishing
    if (badUsb.getState() == BadUSB::State::FINISHED && millis() - entryTime_ > 2000) {
        badUsb.stopScript(); // Clean up
        app->returnToMenu(MenuType::BAD_USB_SCRIPT_LIST);
    }
}

void BadUSBActiveMenu::handleInput(App* app, InputEvent event) {
    if (event == InputEvent::BTN_BACK_PRESS || event == InputEvent::BTN_OK_PRESS) {
        app->getBadUSB().stopScript();
        app->returnToMenu(MenuType::BAD_USB_SCRIPT_LIST);
    }
}

void BadUSBActiveMenu::draw(App* app, U8G2& display) {
    auto& badUsb = app->getBadUSB();

    const char* scriptName = badUsb.getScriptName().c_str();
    const char* status = "";
    
    switch(badUsb.getState()) {
        case BadUSB::State::WAITING_FOR_CONNECTION: status = "Waiting for Host..."; break;
        case BadUSB::State::RUNNING:                status = "Running..."; break;
        case BadUSB::State::FINISHED:               status = "Finished!"; break;
        default:                                    status = "Idle"; break;
    }

    display.setFont(u8g2_font_7x13B_tr);
    char* truncatedName = truncateText(scriptName, 124, display);
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(truncatedName)) / 2, 24, truncatedName);
    
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(status)) / 2, 38, status);

    if (badUsb.getState() == BadUSB::State::RUNNING) {
        char lineInfo[32];
        snprintf(lineInfo, sizeof(lineInfo), "Line: %u", badUsb.getLinesExecuted());
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(lineInfo)) / 2, 50, lineInfo);
    }
    
    display.setFont(u8g2_font_5x7_tf);
    const char* instruction = "Press BACK to Stop";
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(instruction)) / 2, 63, instruction);
}