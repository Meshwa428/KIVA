#ifndef FIRMWARE_LIST_MENU_H
#define FIRMWARE_LIST_MENU_H

#include "IMenu.h"
#include "Animation.h"
#include "Firmware.h"
#include <vector>

class FirmwareListMenu : public IMenu {
public:
    FirmwareListMenu();

    void onEnter(App* app, bool isForwardNav) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(InputEvent event, App* app) override;

    const char* getTitle() const override { return "Update from SD"; }
    MenuType getMenuType() const override { return MenuType::FIRMWARE_LIST_SD; }

private:
    void scroll(int direction);
    
    struct DisplayItem {
        std::string label;
        bool isBackButton;
        int firmwareIndex;
    };
    
    std::vector<DisplayItem> displayItems_;
    int selectedIndex_;
    VerticalListAnimation animation_;

    // --- NEW: Marquee State (Correctly added here) ---
    char marqueeText_[40];
    int marqueeTextLenPx_;
    float marqueeOffset_;
    unsigned long lastMarqueeTime_;
    bool marqueeActive_;
    bool marqueePaused_;
    unsigned long marqueePauseStartTime_;
    bool marqueeScrollLeft_;
};

#endif // FIRMWARE_LIST_MENU_H