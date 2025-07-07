#ifndef APP_H
#define APP_H

#include "HardwareManager.h"
#include "IMenu.h"
#include "MainMenu.h"
#include "CarouselMenu.h"
#include "GridMenu.h"
#include <vector>
#include <map>
#include "WifiManager.h"
#include "WifiListMenu.h"
#include "TextInputMenu.h"
#include "ConnectionStatusMenu.h"

class App {
public:
    App();
    void setup();
    void loop();

    void changeMenu(MenuType type, bool isForwardNav = true);
    HardwareManager& getHardwareManager() { return hardware_; }
    WifiManager& getWifiManager() { return wifiManager_; }
    TextInputMenu& getTextInputMenu() { return textInputMenu_; }
    const std::vector<MenuType>& getNavigationStack() const { return navigationStack_; }

private:
    void drawStatusBar();
    void drawSecondaryDisplay();
    
    // --- REVISED BOOT SEQUENCE METHODS & VARS ---
    void updateAndDrawBootScreen(unsigned long bootStartTime, unsigned long totalBootDuration);
    void logToSmallDisplay(const char* message, const char* status = nullptr);
    
    // These variables are only used during the setup() boot sequence.
    // They are not needed after setup completes.
    float currentProgressBarFillPx_;

    static const int MAX_LOG_LINES_SMALL_DISPLAY = 4;
    static const int MAX_LOG_LINE_LENGTH_SMALL_DISPLAY = 32;
    char smallDisplayLogBuffer_[MAX_LOG_LINES_SMALL_DISPLAY][MAX_LOG_LINE_LENGTH_SMALL_DISPLAY];
    // --- END REVISED VARS ---

    HardwareManager hardware_;
    WifiManager wifiManager_;
    
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
    WifiListMenu wifiListMenu_;
    TextInputMenu textInputMenu_;
    ConnectionStatusMenu connectionStatusMenu_;
};

#endif