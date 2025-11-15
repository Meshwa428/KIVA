#ifndef GRID_MENU_H
#define GRID_MENU_H

#include "IMenu.h"
#include "Animation.h" // Now includes GridAnimation
#include <vector>
#include <string>

class GridMenu : public IMenu {
public:
    // --- CONSTRUCTOR MODIFIED ---
    GridMenu(std::string title, MenuType menuType, std::vector<MenuItem> items, int columns = 2);

    void onEnter(App* app, bool isForwardNav) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(InputEvent event, App* app) override;

    const char* getTitle() const override { return title_.c_str(); }
    MenuType getMenuType() const override { return menuType_; }

private:
    void scroll(int direction);

    std::string title_;
    std::vector<MenuItem> menuItems_;
    MenuType menuType_;
    int selectedIndex_;
    int columns_;

    // Animation state is now encapsulated
    GridAnimation animation_;

    // Marquee state
    char marqueeText_[40];
    int marqueeTextLenPx_;
    float marqueeOffset_;
    unsigned long lastMarqueeTime_;
    bool marqueeActive_;
    bool marqueePaused_;
    unsigned long marqueePauseStartTime_;
    bool marqueeScrollLeft_;

    // State for continuous scrolling
    bool isScrolling_;
    int scrollDirection_; // Corresponds to the 'direction' in scroll()
    unsigned long pressStartTime_;
    unsigned long lastScrollTime_;
};

#endif // GRID_MENU_H