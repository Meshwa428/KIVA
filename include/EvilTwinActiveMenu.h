#ifndef EVIL_TWIN_ACTIVE_MENU_H
#define EVIL_TWIN_ACTIVE_MENU_H

#include "IMenu.h"
#include <vector> // <-- Add include
#include <string> // <-- Add include

class EvilTwinActiveMenu : public IMenu {
public:
    EvilTwinActiveMenu();

    void onEnter(App* app, bool isForwardNav) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(App* app, InputEvent event) override;

    bool drawCustomStatusBar(App* app, U8G2& display) override;

    const char* getTitle() const override { return "Evil Twin"; } // Shorter title
    MenuType getMenuType() const override { return MenuType::EVIL_TWIN_ACTIVE; }

private:
    // --- NEW: State for the victim list ---
    int topDisplayIndex_;
    int selectedIndex_;
    size_t lastKnownVictimCount_;
};

#endif // EVIL_TWIN_ACTIVE_MENU_H