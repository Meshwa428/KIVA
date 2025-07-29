#include "DeauthActiveMenu.h"
#include "App.h"
#include "Deauther.h"
#include "UI_Utils.h"
#include <vector> // <-- ADD THIS INCLUDE for the font vector

DeauthActiveMenu::DeauthActiveMenu() {}

void DeauthActiveMenu::onEnter(App* app, bool isForwardNav) {
    auto& deauther = app->getDeauther();
    if (deauther.isAttackPending()) {
        const auto& config = deauther.getPendingConfig();
        if (config.target == DeauthTarget::ALL_APS) {
            deauther.startAllAPs();
        }
        // For SPECIFIC_AP, start() is called by WifiListDataSource.
    }
}

void DeauthActiveMenu::onExit(App* app) {
    app->getDeauther().stop();
}

void DeauthActiveMenu::onUpdate(App* app) {
    // Deauther::loop is called from App::loop
}

void DeauthActiveMenu::handleInput(App* app, InputEvent event) {
    if (event == InputEvent::BTN_BACK_PRESS || event == InputEvent::BTN_OK_PRESS) {
        app->returnToMenu(MenuType::DEAUTH_TOOLS_GRID);
    }
}

bool DeauthActiveMenu::drawCustomStatusBar(App* app, U8G2& display) {
    auto& deauther = app->getDeauther();
    const auto& config = deauther.getPendingConfig();

    display.setFont(u8g2_font_6x10_tf);
    display.setDrawColor(1);

    // Left side: Attack Method
    const char* modeText = (config.mode == DeauthMode::ROGUE_AP) ? "Rogue AP" : "Broadcast";
    display.drawStr(2, 8, modeText);

    // Right side: Attack Scope
    const char* scopeText = (config.target == DeauthTarget::ALL_APS) ? "Target: All" : "Target: 1";
    int textWidth = display.getStrWidth(scopeText);
    display.drawStr(128 - textWidth - 2, 8, scopeText);
    
    // Bottom line
    display.drawLine(0, STATUS_BAR_H - 1, 127, STATUS_BAR_H - 1);

    return true; // We handled it.
}

void DeauthActiveMenu::draw(App* app, U8G2& display) {
    auto& deauther = app->getDeauther();
    
    // Handle initial states before an attack is fully running
    if (!deauther.isActive()) {
        const char* msg = "Initializing...";
        display.setFont(u8g2_font_7x13B_tr);
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(msg)) / 2, 38, msg);
        return;
    }
    
    const auto& config = deauther.getPendingConfig();
    if (config.target == DeauthTarget::ALL_APS && deauther.getCurrentTargetSsid().empty()) {
        // --- THIS IS THE FIX ---
        // Replace the simple drawStr with the word-wrapping utility
        const char* msg = "Scanning for targets...";
        
        // Define the area on the screen for the message to be centered and wrapped within.
        int textAreaX = 4;
        int textAreaY = 25;
        int textAreaW = 120; // 128px width minus 4px padding on each side
        int textAreaH = 30;
        
        // Define the font to use for wrapping.
        std::vector<const uint8_t*> fonts = {u8g2_font_6x10_tf};
        
        // Call the utility function to draw the wrapped text.
        drawWrappedText(display, msg, textAreaX, textAreaY, textAreaW, textAreaH, fonts);
        // --- END OF FIX ---
        return;
    }

    // --- Refined Main Drawing Area ---
    
    // 1. Attack Mode Title (using a smaller font)
    display.setFont(u8g2_font_6x10_tf);
    display.setDrawColor(1);
    const char* title = (config.mode == DeauthMode::ROGUE_AP) ? "Rogue AP Attack" : "Broadcast Deauth";
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(title)) / 2, 28, title);

    // 2. Current Target SSID (large, bold, and centered)
    display.setFont(u8g2_font_7x13B_tr);
    std::string targetSsid = deauther.getCurrentTargetSsid();
    if (targetSsid.empty()) {
        targetSsid = "---"; // Placeholder if SSID is not yet set
    }
    char* truncated = truncateText(targetSsid.c_str(), 124, display);
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(truncated)) / 2, 44, truncated);
    
    // 3. Footer Instruction
    display.setFont(u8g2_font_5x7_tf);
    const char* instruction = "Press BACK to Stop";
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(instruction)) / 2, 63, instruction);
}