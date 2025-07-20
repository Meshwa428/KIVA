#ifndef EVIL_TWIN_ACTIVE_MENU_H
#define EVIL_TWIN_ACTIVE_MENU_H

#include "IMenu.h"

class EvilTwinActiveMenu : public IMenu {
public:
    EvilTwinActiveMenu();

    void onEnter(App* app) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(App* app, InputEvent event) override;

    const char* getTitle() const override { return "Evil Twin Active"; }
    MenuType getMenuType() const override { return MenuType::EVIL_TWIN_ACTIVE; }
};

#endif // EVIL_TWIN_ACTIVE_MENU_H