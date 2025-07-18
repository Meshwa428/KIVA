#ifndef GRID_MENU_H
#define GRID_MENU_H

#include "IMenu.h"
#include <vector>
#include <string>

class GridMenu : public IMenu {
public:
    GridMenu(std::string title, std::vector<MenuItem> items, int columns = 2);

    void onEnter(App* app) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(App* app, InputEvent event) override;

    const char* getTitle() const override { return title_.c_str(); }
    MenuType getMenuType() const override { return menuType_; }

private:
    void scroll(int direction);
    void startGridAnimation();

    std::string title_;
    std::vector<MenuItem> menuItems_;
    MenuType menuType_;
    int selectedIndex_;
    int columns_;

    // Animation state
    float targetGridScrollOffset_Y_;
    float currentGridScrollOffset_Y_anim_;
    float gridItemScale_[MAX_GRID_ITEMS];
    unsigned long gridItemAnimStartTime_[MAX_GRID_ITEMS];
    bool gridAnimatingIn_;

    // Marquee state
    char marqueeText_[40];
    int marqueeTextLenPx_;
    float marqueeOffset_;
    unsigned long lastMarqueeTime_;
    bool marqueeActive_;
    bool marqueePaused_;
    unsigned long marqueePauseStartTime_;
    bool marqueeScrollLeft_;
};

#endif // GRID_MENU_H