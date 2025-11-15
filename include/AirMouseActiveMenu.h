#ifndef AIR_MOUSE_ACTIVE_MENU_H
#define AIR_MOUSE_ACTIVE_MENU_H

#include "IMenu.h"
#include "AirMouseService.h" // For AirMouseService::Mode enum

class AirMouseActiveMenu : public IMenu {
public:
    AirMouseActiveMenu();

    void onEnter(App* app, bool isForwardNav) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(InputEvent event, App* app) override;

    const char* getTitle() const override { return "Air Mouse"; }
    MenuType getMenuType() const override { return MenuType::AIR_MOUSE_ACTIVE; }
    uint32_t getResourceRequirements() const override;

    void setMode(AirMouseService::Mode mode);

private:
    AirMouseService::Mode modeToStart_;
    unsigned long hapticFeedbackUntil_;

    // --- NEW: State variables for performance mode ---
    bool uiIsPaused_;
    bool wasConnected_;
};

#endif // AIR_MOUSE_ACTIVE_MENU_H