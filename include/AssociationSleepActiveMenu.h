#ifndef ASSOCIATION_SLEEP_ACTIVE_MENU_H
#define ASSOCIATION_SLEEP_ACTIVE_MENU_H

#include "IMenu.h"

class AssociationSleepActiveMenu : public IMenu {
public:
    AssociationSleepActiveMenu();

    void onEnter(App* app, bool isForwardNav) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(InputEvent event, App* app) override;

    const char* getTitle() const override { return "Assoc Sleep"; }
    MenuType getMenuType() const override { return MenuType::ASSOCIATION_SLEEP_ACTIVE; }
};

#endif // ASSOCIATION_SLEEP_ACTIVE_MENU_H
