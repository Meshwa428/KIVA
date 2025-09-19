#ifndef INFO_MENU_H
#define INFO_MENU_H

#include "IMenu.h"
#include "Animation.h" // Add this for scrolling animations
#include <vector>
#include <string>

class InfoMenu : public IMenu {
public:
    InfoMenu();

    void onEnter(App* app, bool isForwardNav) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(InputEvent event, App* app) override;

    const char* getTitle() const override { return "System Info"; }
    MenuType getMenuType() const override { return MenuType::INFO_MENU; }

private:
    void scroll(int direction); // Add scroll helper method

    std::vector<std::pair<std::string, std::string>> infoItems_;
    int uptimeItemIndex_;
    int temperatureItemIndex_; // Add index for temperature item

    // Add state for scrolling
    int selectedIndex_;
    VerticalListAnimation animation_;
};

#endif // INFO_MENU_H