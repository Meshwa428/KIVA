#ifndef CAROUSEL_MENU_H
#define CAROUSEL_MENU_H

#include "IMenu.h"
#include "Animation.h"
#include <vector>
#include <string>

class CarouselMenu : public IMenu {
public:
    CarouselMenu(std::string title, std::vector<MenuItem> items);

    void onEnter(App* app, bool isForwardNav) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(App* app, InputEvent event) override;
    
    const char* getTitle() const override { return title_.c_str(); }
    MenuType getMenuType() const override { return menuType_; }

private:
    void scroll(int direction);
    
    std::string title_;
    std::vector<MenuItem> menuItems_;
    MenuType menuType_;
    int selectedIndex_;
    CarouselAnimation animation_;
    
    // Marquee State
    char marqueeText_[40];
    int marqueeTextLenPx_;
    float marqueeOffset_;
    unsigned long lastMarqueeTime_;
    bool marqueeActive_;
    bool marqueePaused_;
    unsigned long marqueePauseStartTime_;
    bool marqueeScrollLeft_; // ** NEW **
};

#endif // CAROUSEL_MENU_H