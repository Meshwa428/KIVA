#ifndef BEACON_SPAM_ACTIVE_MENU_H
#define BEACON_SPAM_ACTIVE_MENU_H

#include "IMenu.h"
#include "BeaconSpammer.h" // For BeaconSsidMode

class BeaconSpamActiveMenu : public IMenu {
public:
    BeaconSpamActiveMenu();

    void onEnter(App* app) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(App* app, InputEvent event) override;

    void setAttackParameters(BeaconSsidMode mode, const std::string& filePath = "");

    const char* getTitle() const override { return "Beacon Spam Active"; }
    MenuType getMenuType() const override { return MenuType::BEACON_SPAM_ACTIVE; }

private:
    BeaconSsidMode modeToStart_;
    std::string filePathToUse_;
};

#endif // BEACON_SPAM_ACTIVE_MENU_H