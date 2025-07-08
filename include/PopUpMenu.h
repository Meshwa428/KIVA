#ifndef POPUP_MENU_H
#define POPUP_MENU_H

#include "IMenu.h"
#include <functional> // For std::function

class PopUpMenu : public IMenu {
public:
    // Callback takes the app context so it can trigger actions
    using OnConfirmCallback = std::function<void(App*)>;

    PopUpMenu();

    // --- IMenu Interface ---
    void onEnter(App* app) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(App* app, InputEvent event) override;

    const char* getTitle() const override; // Title is dynamic for the status bar
    MenuType getMenuType() const override { return MenuType::POPUP; } // <-- Use new enum value

    // --- Configuration ---
    void configure(std::string title,
                   std::string message,
                   OnConfirmCallback onConfirm,
                   std::string confirmText = "OK",
                   std::string cancelText = "Cancel");

private:
    // Configuration for the current pop-up
    std::string title_;
    std::string message_;
    std::string confirmText_;
    std::string cancelText_;
    OnConfirmCallback onConfirm_;

    // UI State
    int selectedOption_; // 0 for cancel, 1 for confirm

    // Animation State
    float overlayScale_;
};

#endif // POPUP_MENU_H