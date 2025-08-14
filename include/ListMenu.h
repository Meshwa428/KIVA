#ifndef LIST_MENU_H
#define LIST_MENU_H

#include "IMenu.h"
#include "Animation.h"
#include "IListMenuDataSource.h"
#include <string>

class ListMenu : public IMenu {
public:
    ListMenu(std::string title, MenuType menuType, IListMenuDataSource* dataSource);

    void onEnter(App* app, bool isForwardNav) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(App* app, InputEvent event) override;

    const char* getTitle() const override { return title_.c_str(); }
    MenuType getMenuType() const override { return menuType_; }

    void reloadData(App* app, bool resetSelection = true);
    int getSelectedIndex() const { return selectedIndex_; }
    
    // --- NEW: Public helper for marquee text ---
    void updateAndDrawText(U8G2& display, const char* text, int x, int y, int availableWidth, bool isSelected);

private:
    void scroll(int direction);
    
    std::string title_;
    MenuType menuType_;
    IListMenuDataSource* dataSource_;
    
    int selectedIndex_;
    int totalItems_;
    VerticalListAnimation animation_;
    
    // Marquee State - owned by the ListMenu
    char marqueeText_[64];
    int marqueeTextLenPx_;
    float marqueeOffset_;
    unsigned long lastMarqueeTime_;
    bool marqueeActive_;
    bool marqueePaused_;
    unsigned long marqueePauseStartTime_;
    bool marqueeScrollLeft_;
};

#endif // LIST_MENU_H