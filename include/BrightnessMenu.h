// KIVA/include/BrightnessMenu.h

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
    void handleInput(InputEvent event, App* app) override;

    const char* getTitle() const override { return "Brightness"; }
    MenuType getMenuType() const override { return MenuType::BRIGHTNESS_MENU; }

private:
    void changeBrightness(App* app, int delta);

    // --- UI State ---
    int selectedIndex_; // 0 for Main, 1 for Aux

    // --- Animation State ---
    VerticalListAnimation animation_;

    // --- NEW: State for continuous adjustment ---
    bool isAdjusting_;
    int adjustDirection_; // -1 for decrease, 1 for increase
    unsigned long pressStartTime_;
    unsigned long lastAdjustTime_;
};

#endif // BRIGHTNESS_MENU_H