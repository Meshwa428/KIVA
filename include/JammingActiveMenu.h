#ifndef JAMMING_ACTIVE_MENU_H
#define JAMMING_ACTIVE_MENU_H

#include "IMenu.h"
#include <RF_Sweeper.h> // Use the new library's header

class JammingActiveMenu : public IMenu {
public:
    JammingActiveMenu();

    void onEnter(App* app, bool isForwardNav) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(App* app, InputEvent event) override;

    // Use the new library's enums and config struct
    void setJammingModeToStart(JammingMode mode);
    void setJammingConfig(const JammerConfig& config);

    const char* getTitle() const override { return "Jammer Active"; }
    MenuType getMenuType() const override { return MenuType::JAMMING_ACTIVE; }

private:
    JammingMode modeToStart_;     
    JammerConfig startConfig_;
};

#endif // JAMMING_ACTIVE_MENU_H