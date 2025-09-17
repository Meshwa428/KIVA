#ifndef BLE_SPAM_ACTIVE_MENU_H
#define BLE_SPAM_ACTIVE_MENU_H

#include "IMenu.h"
#include "BleSpammer.h" // For BleSpamMode

class BleSpamActiveMenu : public IMenu {
public:
    BleSpamActiveMenu();

    void onEnter(App* app, bool isForwardNav) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(InputEvent event, App* app) override;

    void setSpamModeToStart(BleSpamMode mode);

    const char* getTitle() const override { return "BLE Spam Active"; }
    MenuType getMenuType() const override { return MenuType::BLE_SPAM_ACTIVE; }

private:
    BleSpamMode modeToStart_;
};

#endif // BLE_SPAM_ACTIVE_MENU_H