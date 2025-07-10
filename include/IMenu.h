#ifndef I_MENU_H
#define I_MENU_H

#include "Config.h"
#include "Icons.h"
#include <vector>
#include <string>

// Forward declarations to prevent circular dependencies
class App;
class U8G2;

struct MenuItem {
    const char* label;
    IconType icon;
    MenuType targetMenu; 
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
    
    virtual const char* getTitle() const = 0;
    virtual MenuType getMenuType() const = 0;
};

#endif // I_MENU_H