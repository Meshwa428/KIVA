#ifndef PROBE_SNIFFER_ACTIVE_MENU_H
#define PROBE_SNIFFER_ACTIVE_MENU_H

#include "IMenu.h"
#include <vector>
#include <string>

class ProbeSnifferActiveMenu : public IMenu {
public:
    ProbeSnifferActiveMenu();

    void onEnter(App* app, bool isForwardNav) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(InputEvent event, App* app) override;

    // --- NEW: Override for custom status bar ---
    bool drawCustomStatusBar(App* app, U8G2& display) override;

    const char* getTitle() const override { return "Probe Sniffer"; }
    MenuType getMenuType() const override { return MenuType::PROBE_ACTIVE; }

private:
    std::vector<std::string> displaySsids_;
    int topDisplayIndex_;
    int selectedIndex_; // <-- ADD THIS for visual highlight
    size_t lastKnownSsidCount_;
};

#endif // PROBE_SNIFFER_ACTIVE_MENU_H