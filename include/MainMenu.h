#ifndef MAIN_MENU_H
#define MAIN_MENU_H

#include "IMenu.h"
#include "Animation.h"
#include <vector>

class MainMenu : public IMenu {
public:
    MainMenu();

    void onEnter(App* app, bool isForwardNav) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(InputEvent event, App* app) override;
    
    // --- IMPLEMENTING THE NEW METHOD ---
    const char* getTitle() const override { return "KivaOS"; } // Main menu has a special title
    MenuType getMenuType() const override { return MenuType::MAIN; }

private:
    void scroll(int direction);
    
    std::vector<MenuItem> menuItems_;
    int selectedIndex_;
    VerticalListAnimation animation_;
};

#endif // MAIN_MENU_H