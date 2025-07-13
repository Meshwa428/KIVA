#ifndef POPUP_MENU_H
#define POPUP_MENU_H

#include "IMenu.h"
#include <functional> // For std::function

class PopUpMenu : public IMenu {
public:
    using OnConfirmCallback = std::function<void(App*)>;

    PopUpMenu();

    void onEnter(App* app) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(App* app, InputEvent event) override;

    const char* getTitle() const override;
    MenuType getMenuType() const override { return MenuType::POPUP; }

    // --- The CORRECT declaration with the optional boolean ---
    void configure(std::string title,
                   std::string message,
                   OnConfirmCallback onConfirm,
                   std::string confirmText = "OK",
                   std::string cancelText = "Cancel",
                   bool executeOnConfirmBeforeExit = false); // Default to the OTA-safe behavior

private:
    std::string title_;
    std::string message_;
    std::string confirmText_;
    std::string cancelText_;
    OnConfirmCallback onConfirm_;
    
    // --- Flag to control execution order ---
    bool executeOnConfirmBeforeExit_;

    int selectedOption_;
    float overlayScale_;
};

#endif // POPUP_MENU_H