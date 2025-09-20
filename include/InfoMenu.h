#ifndef INFO_MENU_H
#define INFO_MENU_H

#include "IMenu.h"
#include "Config.h" // For SecondaryWidgetType
#include "Animation.h"
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
    struct InfoItem {
        std::string label;
        std::string value;
        bool isToggleable;
        SecondaryWidgetType widgetType;
    };

    void scroll(int direction);
    void buildInfoItems(App* app);

    std::vector<InfoItem> infoItems_;
    int selectedIndex_;
    VerticalListAnimation animation_;
};

#endif // INFO_MENU_H