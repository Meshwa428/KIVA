#ifndef INFO_MENU_H
#define INFO_MENU_H

#include "IMenu.h"
#include <vector>
#include <string>

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
    std::vector<std::pair<std::string, std::string>> infoItems_;
    int uptimeItemIndex_; // To efficiently update the uptime string
};

#endif // INFO_MENU_H