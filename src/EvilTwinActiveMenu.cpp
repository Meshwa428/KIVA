#include "EvilTwinActiveMenu.h"
#include "App.h"
#include "EvilTwin.h" // The attack module
#include "UI_Utils.h" // For truncateText

EvilTwinActiveMenu::EvilTwinActiveMenu() {}

void EvilTwinActiveMenu::onEnter(App* app) {}

void EvilTwinActiveMenu::onExit(App* app) {
    app->getEvilTwin().stop();
}

void EvilTwinActiveMenu::onUpdate(App* app) {}

void EvilTwinActiveMenu::handleInput(App* app, InputEvent event) {
    if (event == InputEvent::BTN_BACK_PRESS || event == InputEvent::BTN_OK_PRESS) {
        app->returnToMenu(MenuType::WIFI_TOOLS_GRID);
    }
}

void EvilTwinActiveMenu::draw(App* app, U8G2& display) {
    auto& evilTwin = app->getEvilTwin();

    if (!evilTwin.isActive()) {
        const char* msg = "Initializing...";
        display.setFont(u8g2_font_7x13B_tr);
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(msg)) / 2, 38, msg);
        return;
    }

    // --- Main Drawing Logic ---
    display.setFont(u8g2_font_7x13B_tr);
    const char* titleMsg = "Evil Twin Active";
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(titleMsg)) / 2, 24, titleMsg);

    display.setFont(u8g2_font_6x10_tf);
    
    // Display Target SSID
    std::string target_line = "Target: ";
    target_line += evilTwin.getTargetNetwork().ssid;
    char* truncated_target = truncateText(target_line.c_str(), 124, display);
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(truncated_target)) / 2, 38, truncated_target);

    // --- MODIFIED UI: Display Victim Count with a better font ---
    int victimCount = evilTwin.getVictimCount();

    if (victimCount > 0) {
        // --- THIS IS THE CHANGE ---
        // Switched from logisoso16 to profont17, which is cleaner and slightly smaller.
        display.setFont(u8g2_font_profont17_tr);
        
        char countStr[20];
        snprintf(countStr, sizeof(countStr), "Victims: %d", victimCount);

        // Center the text vertically in the remaining space for a polished look.
        // Top of text area is ~42px, bottom is ~60px. Center is ~51px.
        // profont17 has an ascent of 12, so y-baseline should be 51 + (12/2) = ~57.
        // Let's adjust for perfect centering.
        display.setFont(u8g2_font_6x10_tf);
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(countStr)) / 2, 52, countStr);
    } else {
        // If no victims yet, show the "waiting" status
        const char* status_msg = "Waiting for victim...";
        display.setFont(u8g2_font_6x10_tf);
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(status_msg)) / 2, 52, status_msg);
    }

    display.setFont(u8g2_font_5x7_tf);
    const char* instruction = "Press BACK to Stop";
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(instruction)) / 2, 63, instruction);
}