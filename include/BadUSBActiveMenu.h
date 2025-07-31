#ifndef BAD_USB_ACTIVE_MENU_H
#define BAD_USB_ACTIVE_MENU_H

#include "IMenu.h"

class BadUSBActiveMenu : public IMenu {
public:
    BadUSBActiveMenu();

    void onEnter(App* app, bool isForwardNav) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(App* app, InputEvent event) override;

    const char* getTitle() const override { return "BadUSB Active"; }
    MenuType getMenuType() const override { return MenuType::BAD_USB_ACTIVE; }

private:
    unsigned long entryTime_;
};

#endif // BAD_USB_ACTIVE_MENU_H