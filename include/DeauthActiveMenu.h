#ifndef DEAUTH_ACTIVE_MENU_H
#define DEAUTH_ACTIVE_MENU_H

#include "IMenu.h"

class DeauthActiveMenu : public IMenu {
public:
    DeauthActiveMenu();

    void onEnter(App* app) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(App* app, InputEvent event) override;

    const char* getTitle() const override { return "Deauther Active"; }
    MenuType getMenuType() const override { return MenuType::DEAUTH_ACTIVE; }
};

#endif // DEAUTH_ACTIVE_MENU_H