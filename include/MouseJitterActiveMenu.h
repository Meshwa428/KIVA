#ifndef MOUSE_JITTER_ACTIVE_MENU_H
#define MOUSE_JITTER_ACTIVE_MENU_H

#include "IMenu.h"
#include <HIDForge.h>

class MouseJitterActiveMenu : public IMenu {
public:
    enum class JitterMode { USB, BLE };

    MouseJitterActiveMenu();

    void onEnter(App* app, bool isForwardNav) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(App* app, InputEvent event) override;

    void setJitterMode(JitterMode mode);

    const char* getTitle() const override { return "Mouse Jitter"; }
    MenuType getMenuType() const override { return MenuType::MOUSE_JITTER_ACTIVE; }

private:
    JitterMode mode_;
    MouseInterface* activeMouse_; // Pointer to the active mouse object
};

#endif // MOUSE_JITTER_ACTIVE_MENU_H