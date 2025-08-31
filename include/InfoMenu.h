#ifndef INFO_MENU_H
#define INFO_MENU_H

#include "IMenu.h"
#include "Animation.h" // For list animation

class InfoMenu : public IMenu {
public:
    InfoMenu();

    void onEnter(App* app, bool isForwardNav) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(App* app, InputEvent event) override;

    const char* getTitle() const override { return "System Info"; }
    MenuType getMenuType() const override { return MenuType::INFO_MENU; }

private:
    struct InfoItem {
        std::string label;
        std::string value;
    };

    std::vector<InfoItem> infoItems_;
    int selectedIndex_; // For scrolling
    VerticalListAnimation animation_;
};

#endif // INFO_MENU_H