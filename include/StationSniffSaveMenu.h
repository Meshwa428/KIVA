#ifndef STATION_SNIFF_SAVE_MENU_H
#define STATION_SNIFF_SAVE_MENU_H

#include "IMenu.h"
#include "WifiManager.h" // For WifiNetworkInfo

class StationSniffSaveMenu : public IMenu {
public:
    StationSniffSaveMenu();

    void onEnter(App* app, bool isForwardNav) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(InputEvent event, App* app) override;

    void setTarget(const WifiNetworkInfo& target);

    const char* getTitle() const override { return "Sniff & Save"; }
    MenuType getMenuType() const override { return MenuType::STATION_SNIFF_SAVE_ACTIVE; }

    uint32_t getResourceRequirements() const override { return (uint32_t)ResourceRequirement::WIFI; }

private:
    WifiNetworkInfo target_;
    size_t lastClientCount_;
};

#endif // STATION_SNIFF_SAVE_MENU_H