#ifndef JAMMING_ACTIVE_MENU_H
#define JAMMING_ACTIVE_MENU_H

#include "IMenu.h"
#include "Jammer.h"

class JammingActiveMenu : public IMenu {
public:
    JammingActiveMenu();

    void onEnter(App* app, bool isForwardNav) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(InputEvent event, App* app) override;

    void setJammingModeToStart(JammingMode mode);
    void setJammingConfig(const JammerConfig& config); // Now takes the full config struct

    const char* getTitle() const override { return "Jammer Active"; }
    MenuType getMenuType() const override { return MenuType::JAMMING_ACTIVE; }

private:
    JammingMode modeToStart_;     
    JammerConfig startConfig_;

    bool initialFrameDrawn_ = false;
};

#endif // JAMMING_ACTIVE_MENU_H