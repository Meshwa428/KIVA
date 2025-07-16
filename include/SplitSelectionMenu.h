#ifndef SPLIT_SELECTION_MENU_H
#define SPLIT_SELECTION_MENU_H

#include "IMenu.h"
#include <vector>
#include <string>

class SplitSelectionMenu : public IMenu {
public:
    SplitSelectionMenu(std::string title, MenuType menuType, const std::vector<MenuItem>& items);

    void onEnter(App* app) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(App* app, InputEvent event) override;

    const char* getTitle() const override { return title_.c_str(); }
    MenuType getMenuType() const override { return menuType_; }

private:
    void scroll(int direction);
    
    std::string title_;
    MenuType menuType_;
    std::vector<MenuItem> menuItems_;
    
    int selectedIndex_;

    // Animation state
    float panelTargetOffsetX_[3];
    float panelCurrentOffsetX_[3];
    float panelTargetScale_[3];
    float panelCurrentScale_[3];
    bool isAnimatingIn_;
    
    // --- NEW: Marquee State Variables ---
    char marqueeText_[64];
    int marqueeTextLenPx_;
    float marqueeOffset_;
    unsigned long lastMarqueeTime_;
    bool marqueeActive_;
    bool marqueePaused_;
    unsigned long marqueePauseStartTime_;
    bool marqueeScrollLeft_;
};

#endif // SPLIT_SELECTION_MENU_H