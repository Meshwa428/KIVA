#include "DeauthActiveMenu.h"
#include "App.h"
#include "Deauther.h"
#include "UI_Utils.h"

DeauthActiveMenu::DeauthActiveMenu() {}

void DeauthActiveMenu::onEnter(App* app) {
    auto& deauther = app->getDeauther();
    if (deauther.isAttackPending()) {
        const auto& config = deauther.getPendingConfig();
        // This case is for attacks that don't require selecting a specific AP from the list.
        if (config.target == DeauthTarget::ALL_APS) {
            if (!deauther.startAllAPs()) {
                app->showPopUp("Error", "No networks found to attack.", nullptr, "OK", "", true);
                app->changeMenu(MenuType::BACK);
            }
        }
        // For SPECIFIC_AP, the start() is called by WifiListDataSource *before* this menu is entered.
        // So, we don't need to do anything here for that case.
    }
}

void DeauthActiveMenu::onExit(App* app) {
    app->getDeauther().stop();
}

void DeauthActiveMenu::onUpdate(App* app) {
    // The main App::loop() handles throttling, and Deauther::loop() handles the attack logic.
}

void DeauthActiveMenu::handleInput(App* app, InputEvent event) {
    // --- THIS IS THE FIX ---
    // Instead of just going "back" one step to the list, we return to the menu
    // where the attack type was originally selected.
    if (event == InputEvent::BTN_BACK_PRESS || event == InputEvent::BTN_OK_PRESS) {
        app->returnToMenu(MenuType::DEAUTH_TOOLS_GRID); // onExit will be called automatically, stopping the attack.
    }
}

void DeauthActiveMenu::draw(App* app, U8G2& display) {
    auto& deauther = app->getDeauther();
    if (!deauther.isActive()) {
        const char* msg = "Starting...";
        display.setFont(u8g2_font_7x13B_tr);
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(msg)) / 2, 38, msg);
        return;
    }

    // Now we get the *active* config, not the pending one.
    const auto& config = deauther.getPendingConfig();
    
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