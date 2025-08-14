#ifndef SETTINGS_MENU_H
#define SETTINGS_MENU_H

#include "IMenu.h"
#include "Animation.h"
#include <vector>
#include <string>
#include <functional> // Required for std::function

class SettingsMenu : public IMenu {
public:
    SettingsMenu();

    void onEnter(App* app, bool isForwardNav) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(App* app, InputEvent event) override;

    const char* getTitle() const override { return "Settings"; }
    MenuType getMenuType() const override { return MenuType::SETTINGS; }

private:
    void scroll(int direction);
    void changeBrightness(App* app, int delta, const std::string& target);
    void changeVolume(App* app, int delta); // New helper for volume
    void changeKeyboardLayout(App* app, int direction);

    struct SettingItem {
        std::string label;
        MenuType targetMenu;
        std::function<void(App*)> action;
        bool isSlider;
    };

    std::vector<SettingItem> menuItems_;
    int selectedIndex_;
    VerticalListAnimation animation_;
};

#endif // SETTINGS_MENU_H