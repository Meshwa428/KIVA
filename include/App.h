#ifndef APP_H
#define APP_H

#include "HardwareManager.h"
#include "IMenu.h"
#include "MainMenu.h"
#include "CarouselMenu.h"
#include "GridMenu.h"
#include <vector>
#include <map>

class App {
public:
    App();
    void setup();
    void loop();

    void changeMenu(MenuType type, bool isForwardNav = true);
    HardwareManager& getHardwareManager() { return hardware_; }

private:
    void drawStatusBar();
    void drawSecondaryDisplay(); // --- NEW: Declaration added ---

    HardwareManager hardware_;
    
    std::map<MenuType, IMenu*> menuRegistry_;
    IMenu* currentMenu_;
    
    std::vector<MenuType> navigationStack_;
    
    // Concrete Menu Instances
    MainMenu mainMenu_;
    CarouselMenu toolsMenu_;
    CarouselMenu gamesMenu_;
    CarouselMenu settingsMenu_;
    CarouselMenu utilitiesMenu_;
    GridMenu wifiToolsMenu_;
};

#endif