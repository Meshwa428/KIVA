#include "Event.h"
#include "EventDispatcher.h"
#include "DeauthActiveMenu.h"
#include "App.h"
#include "Deauther.h"
#include "UI_Utils.h"
#include <vector> // <-- ADD THIS INCLUDE for the font vector

DeauthActiveMenu::DeauthActiveMenu() {}

void DeauthActiveMenu::onEnter(App* app, bool isForwardNav) {
    EventDispatcher::getInstance().subscribe(EventType::APP_INPUT, this);
    auto& deauther = app->getDeauther();
    if (deauther.isAttackPending()) {
        const auto& config = deauther.getPendingConfig();
        // --- FIX: Use correct enum ---
        if (config.type == DeauthAttackType::BROADCAST_NORMAL || config.type == DeauthAttackType::BROADCAST_EVIL_TWIN) {
            // --- FIX: Use correct method name ---
            deauther.startBroadcast();
        }
    }
}

void DeauthActiveMenu::onExit(App* app) {
    EventDispatcher::getInstance().unsubscribe(EventType::APP_INPUT, this);
    app->getDeauther().stop();
}

void DeauthActiveMenu::onUpdate(App* app) {
    // Deauther::loop is called from App::loop
}

void DeauthActiveMenu::handleInput(InputEvent event, App* app) {
    if (event == InputEvent::BTN_BACK_PRESS || event == InputEvent::BTN_OK_PRESS) {
        EventDispatcher::getInstance().publish(ReturnToMenuEvent(MenuType::WIFI_ATTACKS_LIST));
    }
}

bool DeauthActiveMenu::drawCustomStatusBar(App* app, U8G2& display) {
    auto& deauther = app->getDeauther();
    const auto& config = deauther.getPendingConfig();

    display.setFont(u8g2_font_6x10_tf);
    display.setDrawColor(1);

    // Left side: Attack Method
    const char* modeText = (
        config.type == DeauthAttackType::EVIL_TWIN || 
        config.type == DeauthAttackType::BROADCAST_EVIL_TWIN) ? "Evil Twin" : "Broadcast";
    display.drawStr(2, 8, modeText);

    // Right side: Attack Scope
    const char* scopeText = (
        config.type == DeauthAttackType::BROADCAST_NORMAL ||
        config.type == DeauthAttackType::BROADCAST_EVIL_TWIN) ? "Target: All" : "Target: 1";
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
    if ((config.type == DeauthAttackType::BROADCAST_NORMAL ||
         config.type == DeauthAttackType::BROADCAST_EVIL_TWIN) && deauther.getCurrentTargetSsid().empty()) {
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
    const char* title = "Broadcast Deauth";
    if (config.type == DeauthAttackType::EVIL_TWIN || config.type == DeauthAttackType::BROADCAST_EVIL_TWIN) {
        title = "Evil Twin";
    } else if (config.type == DeauthAttackType::PINPOINT_CLIENT) {
        title = "Pinpoint Deauth";
    }
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
