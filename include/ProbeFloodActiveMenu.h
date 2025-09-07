#ifndef PROBE_FLOOD_ACTIVE_MENU_H
#define PROBE_FLOOD_ACTIVE_MENU_H

#include "IMenu.h"
#include "ProbeFlooder.h"

class ProbeFloodActiveMenu : public IMenu {
public:
    ProbeFloodActiveMenu();

    void onEnter(App* app, bool isForwardNav) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(InputEvent event, App* app) override;

    // --- NEW: Override for custom status bar ---
    bool drawCustomStatusBar(App* app, U8G2& display) override;

    void setAttackParameters(ProbeFloodMode mode, const std::string& filePath = "");

    const char* getTitle() const override { return "Probe Flood"; }
    MenuType getMenuType() const override { return MenuType::PROBE_FLOOD_ACTIVE; }

private:
    ProbeFloodMode modeToStart_;
    std::string filePathToUse_;
};

#endif // PROBE_FLOOD_ACTIVE_MENU_H