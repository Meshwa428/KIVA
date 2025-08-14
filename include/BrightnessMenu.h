#ifndef BRIGHTNESS_MENU_H
#define BRIGHTNESS_MENU_H

#include "IMenu.h"
#include "Animation.h" // Use the existing VerticalListAnimation

class BrightnessMenu : public IMenu {
public:
    BrightnessMenu();

    void onEnter(App* app, bool isForwardNav) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(App* app, InputEvent event) override;

    const char* getTitle() const override { return "Brightness"; }
    MenuType getMenuType() const override { return MenuType::BRIGHTNESS_MENU; }

private:
    void changeBrightness(App* app, int delta);

    // --- UI State ---
    int selectedIndex_; // 0 for Main, 1 for Aux

    // --- Animation State ---
    VerticalListAnimation animation_;
};

#endif // BRIGHTNESS_MENU_H