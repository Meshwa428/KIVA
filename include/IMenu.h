#ifndef I_MENU_H
#define I_MENU_H

#include "Config.h"
#include "Icons.h"
#include "EventDispatcher.h" // Include the dispatcher
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

    // --- NEW: Optional members for sliders/interactive items ---
    bool isInteractive = false;
    std::function<std::string(App*)> getValue = nullptr;
    std::function<void(App*, int)> adjustValue = nullptr;
};

// A menu is now also an ISubscriber
class IMenu : public ISubscriber {
public:
    virtual ~IMenu() {}

    // Lifecycle methods
    virtual void onEnter(App* app, bool isForwardNav) = 0;
    virtual void onUpdate(App* app) = 0;
    virtual void onExit(App* app) = 0;

    virtual void draw(App* app, U8G2& display) = 0;

    // The onEvent method from ISubscriber interface
    void onEvent(const Event& event) override;
    
    /**
     * @brief [NEW] Draws a custom status bar for this menu.
     * @return true if a custom status bar was drawn, false to use the default one from App.
     */
    virtual bool drawCustomStatusBar(App* app, U8G2& display) { return false; }

    virtual const char* getTitle() const = 0;
    virtual MenuType getMenuType() const = 0;

protected:
    // A new helper method for menus to implement
    virtual void handleInput(InputEvent event, App* app) = 0;
};

#endif // I_MENU_H