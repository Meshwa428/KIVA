#ifndef CONNECTION_STATUS_MENU_H
#define CONNECTION_STATUS_MENU_H

#include "IMenu.h"

class ConnectionStatusMenu : public IMenu {
public:
    ConnectionStatusMenu();
    void onEnter(App* app, bool isForwardNav) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(InputEvent event, App* app) override;

    const char* getTitle() const override { return "Connection Status"; }
    MenuType getMenuType() const override { return MenuType::WIFI_CONNECTION_STATUS; }

    uint32_t getResourceRequirements() const override { return (uint32_t)ResourceRequirement::WIFI; }

private:
    unsigned long successTime_; // Will be 0 until a connection is made
    unsigned long failTime_; 
};

#endif // CONNECTION_STATUS_MENU_H