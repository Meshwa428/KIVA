#ifndef USB_DRIVE_MENU_H
#define USB_DRIVE_MENU_H

#include "IMenu.h"
#include <memory> // Required for std::unique_ptr

// Forward declarations for HIDForge classes
class UsbMsc;
class SDCard;

class UsbDriveMenu : public IMenu {
public:
    UsbDriveMenu();

    void onEnter(App* app, bool isForwardNav) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(App* app, InputEvent event) override;

    const char* getTitle() const override { return "USB Drive Mode"; }
    MenuType getMenuType() const override { return MenuType::USB_DRIVE_MODE; }
    
private:
    std::unique_ptr<UsbMsc> storage_;
    std::unique_ptr<SDCard> card_;
};

#endif // USB_DRIVE_MODE