#ifndef I_MENU_H
#define I_MENU_H

#include "Config.h"
#include "Icons.h"
#include <vector>
#include <string>
#include <functional> // For std::function

// Forward declarations to prevent circular dependencies
class App;
class U8G2;

struct MenuItem {
    const char* label;
    IconType icon;
    MenuType targetMenu; 
    std::function<void(App*)> action;

    // --- ADD THIS CONSTRUCTOR ---
    // It has a default argument for the action, making it optional.
    MenuItem(const char* l, IconType i, MenuType t, std::function<void(App*)> a = nullptr)
        : label(l), icon(i), targetMenu(t), action(a) {}
};

class IMenu {
public:
    virtual ~IMenu() {}

    // Lifecycle methods
    virtual void onEnter(App* app) = 0;
    virtual void onUpdate(App* app) = 0;
    virtual void onExit(App* app) = 0;

    virtual void draw(App* app, U8G2& display) = 0;
    virtual void handleInput(App* app, InputEvent event) = 0;
    
    /**
     * @brief [NEW] Draws a custom status bar for this menu.
     * @return true if a custom status bar was drawn, false to use the default one from App.
     */
    virtual bool drawCustomStatusBar(App* app, U8G2& display) { return false; }

    virtual const char* getTitle() const = 0;
    virtual MenuType getMenuType() const = 0;
};

#endif // I_MENU_H