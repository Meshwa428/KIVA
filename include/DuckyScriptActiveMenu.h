#ifndef DUCKY_SCRIPT_ACTIVE_MENU_H
#define DUCKY_SCRIPT_ACTIVE_MENU_H

#include "IMenu.h"

class DuckyScriptActiveMenu : public IMenu {
public:
    DuckyScriptActiveMenu();

    void onEnter(App* app, bool isForwardNav) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(InputEvent event, App* app) override;

    const char* getTitle() const override { return "Ducky Script"; }
    MenuType getMenuType() const override { return MenuType::DUCKY_SCRIPT_ACTIVE; }

private:
    unsigned long entryTime_;
};

#endif // DUCKY_SCRIPT_ACTIVE_MENU_H