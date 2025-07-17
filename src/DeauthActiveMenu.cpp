#include "DeauthActiveMenu.h"
#include "App.h"
#include "Deauther.h"
#include "UI_Utils.h"

DeauthActiveMenu::DeauthActiveMenu() {}

void DeauthActiveMenu::onEnter(App* app) {
    auto& deauther = app->getDeauther();
    if (deauther.isAttackPending()) {
        const auto& config = deauther.getPendingConfig();
        if (config.target == DeauthTarget::ALL_APS) {
            // The startAllAPs now correctly initiates a scan.
            // The Deauther::loop will handle the rest.
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

void DeauthActiveMenu::draw(App* app, U8G2& display) {
    auto& deauther = app->getDeauther();

    // --- NEW DRAWING LOGIC ---
    if (!deauther.isActive()) {
        // This case handles the frame before onEnter completes.
        const char* msg = "Initializing...";
        display.setFont(u8g2_font_7x13B_tr);
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(msg)) / 2, 38, msg);
        return;
    }

    const auto& config = deauther.getPendingConfig();

    // If attacking all APs and the target list is still empty, it means we are scanning.
    if (config.target == DeauthTarget::ALL_APS && deauther.getCurrentTargetSsid().empty()) {
        const char* msg = "Scanning...";
        display.setFont(u8g2_font_7x13B_tr);
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(msg)) / 2, 38, msg);
        return;
    }

    // --- Original drawing logic for when attack is running ---
    std::string title = (config.mode == DeauthMode::ROGUE_AP) ? "Rogue AP Attack" : "Broadcast Deauth";
    
    display.setFont(u8g2_font_7x13B_tr);
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(title.c_str())) / 2, 28, title.c_str());

    display.setFont(u8g2_font_6x10_tf);
    std::string target_line = "Target: " + deauther.getCurrentTargetSsid();
    
    char* truncated = truncateText(target_line.c_str(), 124, display);
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(truncated)) / 2, 42, truncated);

    const char* instruction = "Press BACK to Stop";
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(instruction)) / 2, 56, instruction);
}