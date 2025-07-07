#ifndef CONNECTION_STATUS_MENU_H
#define CONNECTION_STATUS_MENU_H

#include "IMenu.h"

class ConnectionStatusMenu : public IMenu {
public:
    ConnectionStatusMenu();
    void onEnter(App* app) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(App* app, InputEvent event) override;

    const char* getTitle() const override { return "Connection Status"; }
    MenuType getMenuType() const override { return MenuType::WIFI_CONNECTION_STATUS; }

private:
    unsigned long entryTime_;
};

#endif // CONNECTION_STATUS_MENU_H