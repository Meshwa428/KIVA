#ifndef OTA_STATUS_MENU_H
#define OTA_STATUS_MENU_H

#include "IMenu.h"

class OtaStatusMenu : public IMenu {
public:
    OtaStatusMenu();

    void onEnter(App* app) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(App* app, InputEvent event) override;

    const char* getTitle() const override;
    MenuType getMenuType() const override { return MenuType::OTA_STATUS; }

private:
    char titleBuffer_[20];
};

#endif // OTA_STATUS_MENU_H