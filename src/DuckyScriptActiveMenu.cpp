#include "DuckyScriptActiveMenu.h"
#include "App.h"
#include "UI_Utils.h"

DuckyScriptActiveMenu::DuckyScriptActiveMenu() : entryTime_(0) {}

void DuckyScriptActiveMenu::onEnter(App* app, bool isForwardNav) {
    entryTime_ = millis();
}

void DuckyScriptActiveMenu::onExit(App* app) {
    if(app->getDuckyRunner().isActive()) {
        app->getDuckyRunner().stopScript();
    }
}

void DuckyScriptActiveMenu::onUpdate(App* app) {
    auto& ducky = app->getDuckyRunner();
    // Use a short delay after finishing before automatically exiting
    if (ducky.getState() == DuckyScriptRunner::State::FINISHED && millis() - entryTime_ > 2000) {
        // stopScript() is called automatically in onExit
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
    // This draw logic is now correct again
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