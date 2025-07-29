#ifndef TEXT_INPUT_MENU_H
#define TEXT_INPUT_MENU_H

#include "IMenu.h"
#include "KeyboardLayout.h"
#include <functional> // For std::function

#define TEXT_INPUT_MAX_LEN 64

class TextInputMenu : public IMenu {
public:
    // --- Type alias for the callback function ---
    using OnSubmitCallback = std::function<void(App*, const char*)>;

    TextInputMenu();
    void onEnter(App* app, bool isForwardNav) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(App* app, InputEvent event) override;

    const char* getTitle() const override { return title_.c_str(); }
    MenuType getMenuType() const override { return MenuType::TEXT_INPUT; }

    // --- Configuration method to be called before changing to this menu ---
    void configure(std::string title, OnSubmitCallback callback, bool maskInput = false, const char* initial_text = "");

private:
    void drawKeyboard(U8G2& display);
    void processKeyPress(int keyValue);
    void moveFocus(int dRow, int dCol);
    
    // --- State for the current input session ---
    std::string title_;
    OnSubmitCallback onSubmit_;
    bool maskInput_;
    
    char inputBuffer_[TEXT_INPUT_MAX_LEN + 1];
    int cursorPosition_;
    
    // --- Keyboard state ---
    KeyboardLayer currentLayer_;
    int focusRow_;
    int focusCol_;
    bool capsLock_;
};

#endif // TEXT_INPUT_MENU_H