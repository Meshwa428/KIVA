#ifndef PROBE_FLOOD_ACTIVE_MENU_H
#define PROBE_FLOOD_ACTIVE_MENU_H

#include "IMenu.h"
#include "ProbeFlooder.h"
#include "WifiManager.h" // Added for WifiNetworkInfo

class ProbeFloodActiveMenu : public IMenu {
public:
    ProbeFloodActiveMenu();

    void onEnter(App* app, bool isForwardNav) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(InputEvent event, App* app) override;

    bool drawCustomStatusBar(App* app, U8G2& display) override;

    // --- CORRECTED OVERLOADS ---
    // This single function now handles all modes that don't take a WifiNetworkInfo struct.
    // The default argument `""` makes it work for modes without a file path.
    void setAttackParameters(ProbeFloodMode mode, const std::string& filePath = "");
    
    // This overload is for the specific PINPOINT_AP mode.
    void setAttackParameters(const WifiNetworkInfo& targetNetwork);

    const char* getTitle() const override { return "Probe Flood"; }
    MenuType getMenuType() const override { return MenuType::PROBE_FLOOD_ACTIVE; }

private:
    ProbeFloodMode modeToStart_;
    std::string filePathToUse_;
    WifiNetworkInfo targetNetwork_;
};

#endif // PROBE_FLOOD_ACTIVE_MENU_H