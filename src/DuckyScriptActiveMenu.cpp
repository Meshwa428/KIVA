#include "DuckyScriptActiveMenu.h"
#include "App.h"
#include "UI_Utils.h"
#include <HIDForge.h> // For USB object

DuckyScriptActiveMenu::DuckyScriptActiveMenu() : entryTime_(0) {}

void DuckyScriptActiveMenu::onEnter(App* app, bool isForwardNav) {
    entryTime_ = millis();
    app->getHardwareManager().setPerformanceMode(true);
}

void DuckyScriptActiveMenu::onExit(App* app) {
    // Stop the script runner, which will release HID interfaces.
    if(app->getDuckyRunner().isActive()) {
        app->getDuckyRunner().stopScript();
    }
    
    app->getHardwareManager().setPerformanceMode(false);
}

void DuckyScriptActiveMenu::onUpdate(App* app) {
    auto& ducky = app->getDuckyRunner();
    if (ducky.getState() == DuckyScriptRunner::State::FINISHED && millis() - entryTime_ > 1500) {
        app->returnToMenu(MenuType::DUCKY_SCRIPT_LIST);
    }
}

void DuckyScriptActiveMenu::handleInput(App* app, InputEvent event) {
    if (event == InputEvent::BTN_BACK_PRESS || event == InputEvent::BTN_OK_PRESS) {
        app->returnToMenu(MenuType::DUCKY_SCRIPT_LIST);
    }
}

void DuckyScriptActiveMenu::draw(App* app, U8G2& display) {
    auto& ducky = app->getDuckyRunner();
    const char* scriptName = ducky.getScriptName().c_str();
    const char* status = "";
    
    switch(ducky.getState()) {
        case DuckyScriptRunner::State::WAITING_FOR_CONNECTION: status = "Waiting for Host..."; break;
        case DuckyScriptRunner::State::RUNNING:                status = "Running..."; break;
        case DuckyScriptRunner::State::FINISHED:               status = "Finished!"; break;
        default:                                               status = "Idle"; break;
    }

    display.setFont(u8g2_font_7x13B_tr);
    char* truncatedName = truncateText(scriptName, 124, display);
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(truncatedName)) / 2, 24, truncatedName);
    
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(status)) / 2, 38, status);

    if (ducky.getState() == DuckyScriptRunner::State::RUNNING) {
        char lineInfo[32];
        snprintf(lineInfo, sizeof(lineInfo), "Line: %u", ducky.getLinesExecuted());
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(lineInfo)) / 2, 50, lineInfo);
    }
    
    display.setFont(u8g2_font_5x7_tf);
    const char* instruction = "Press BACK to Stop";
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(instruction)) / 2, 63, instruction);
}