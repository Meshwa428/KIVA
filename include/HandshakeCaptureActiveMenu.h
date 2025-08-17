#ifndef HANDSHAKE_CAPTURE_ACTIVE_MENU_H
#define HANDSHAKE_CAPTURE_ACTIVE_MENU_H

#include "IMenu.h"

class HandshakeCaptureActiveMenu : public IMenu {
public:
    HandshakeCaptureActiveMenu();
    void onEnter(App* app, bool isForwardNav) override;
    void onExit(App* app) override;
    void onUpdate(App* app) override;
    void handleInput(App* app, InputEvent event) override;
    void draw(App* app, U8G2& display) override;

    const char* getTitle() const override { return "Sniffing..."; }
    MenuType getMenuType() const override { return MenuType::HANDSHAKE_CAPTURE_ACTIVE; }

private:
    unsigned long startTime_;
};

#endif // HANDSHAKE_CAPTURE_ACTIVE_MENU_H
