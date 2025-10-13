#ifndef BAD_MSG_ACTIVE_MENU_H
#define BAD_MSG_ACTIVE_MENU_H

#include "IMenu.h"

class BadMsgActiveMenu : public IMenu {
public:
    BadMsgActiveMenu();

    void onEnter(App* app, bool isForwardNav) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;

    const char* getTitle() const override { return "Bad Msg Attack"; }
    MenuType getMenuType() const override { return MenuType::BAD_MSG_ACTIVE; }

protected:
    void handleInput(InputEvent event, App* app) override;
};

#endif // BAD_MSG_ACTIVE_MENU_H