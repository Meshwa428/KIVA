#ifndef KARMA_ACTIVE_MENU_H
#define KARMA_ACTIVE_MENU_H

#include "IMenu.h"
#include <vector>
#include <string>

class KarmaActiveMenu : public IMenu {
public:
    KarmaActiveMenu();

    void onEnter(App* app, bool isForwardNav) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(InputEvent event, App* app) override;

    bool drawCustomStatusBar(App* app, U8G2& display) override;

    const char* getTitle() const override { return "Karma Attack"; }
    MenuType getMenuType() const override { return MenuType::KARMA_ACTIVE; }

private:
    std::vector<std::string> displaySsids_;
    int topDisplayIndex_;
    int selectedIndex_;
    size_t lastKnownSsidCount_;
    
    bool initialFrameDrawn_ = false;
};

#endif // KARMA_ACTIVE_MENU_H