#ifndef BLE_SPAM_ACTIVE_MENU_H
#define BLE_SPAM_ACTIVE_MENU_H

#include "IMenu.h"
#include <BleSpam.h> // Use the new library's header

class BleSpamActiveMenu : public IMenu {
public:
    BleSpamActiveMenu();

    void onEnter(App* app, bool isForwardNav) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(App* app, InputEvent event) override;

    // Use the new library's enum
    void setSpamType(BleSpam::EBLEPayloadType type);
    void setSpamAll(bool all);

    const char* getTitle() const override { return "BLE Spam Active"; }
    MenuType getMenuType() const override { return MenuType::BLE_SPAM_ACTIVE; }

private:
    BleSpam::EBLEPayloadType spamType_;
    bool spamAll_;
    const char* getSpamTypeString() const;
};

#endif // BLE_SPAM_ACTIVE_MENU_H