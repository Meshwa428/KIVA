#ifndef APP_H
#define APP_H

#include "HardwareManager.h"
#include "IMenu.h"
#include "MainMenu.h"
#include "CarouselMenu.h"
#include "GridMenu.h"
#include <vector>
#include <map>
#include <functional> // For std::function
#include "WifiManager.h"
#include "WifiListMenu.h"
#include "TextInputMenu.h"
#include "ConnectionStatusMenu.h"
#include "PopUpMenu.h"
#include "OtaManager.h"
#include "FirmwareListMenu.h"
#include "OtaStatusMenu.h"

class App {
public:
    App();
    void setup();
    void loop();

    void changeMenu(MenuType type, bool isForwardNav = true);
    void returnToMenu(MenuType type);
    
    void showPopUp(std::string title, std::string message, PopUpMenu::OnConfirmCallback onConfirm);
    HardwareManager& getHardwareManager() { return hardware_; }
    WifiManager& getWifiManager() { return wifiManager_; }
    OtaManager& getOtaManager() { return otaManager_; }
    TextInputMenu& getTextInputMenu() { return textInputMenu_; }
    IMenu* getMenu(MenuType type);

    MenuType getPreviousMenuType() const;

    // --- NEW: For post-wifi connection actions ---
    void setPostWifiAction(std::function<void(App*)> action);
    void executePostWifiAction();
    
    void drawStatusBar(); // <-- MOVED TO PUBLIC

private:
    void drawSecondaryDisplay();
    
    void updateAndDrawBootScreen(unsigned long bootStartTime, unsigned long totalBootDuration);
    void logToSmallDisplay(const char* message, const char* status = nullptr);
    
    float currentProgressBarFillPx_;

    static const int MAX_LOG_LINES_SMALL_DISPLAY = 4;
    static const int MAX_LOG_LINE_LENGTH_SMALL_DISPLAY = 32;
    char smallDisplayLogBuffer_[MAX_LOG_LINES_SMALL_DISPLAY][MAX_LOG_LINE_LENGTH_SMALL_DISPLAY];

    HardwareManager hardware_;
    WifiManager wifiManager_;
    OtaManager otaManager_;
    
    std::map<MenuType, IMenu*> menuRegistry_;
    IMenu* currentMenu_;
    
    std::vector<MenuType> navigationStack_;
    
    // --- NEW ---
    std::function<void(App*)> postWifiAction_ = nullptr;

    // Concrete Menu Instances
    MainMenu mainMenu_;
    CarouselMenu toolsMenu_;
    CarouselMenu gamesMenu_;
    CarouselMenu settingsMenu_;
    CarouselMenu utilitiesMenu_;
    GridMenu wifiToolsMenu_;
    GridMenu firmwareUpdateGrid_;
    WifiListMenu wifiListMenu_;
    TextInputMenu textInputMenu_;
    ConnectionStatusMenu connectionStatusMenu_;
    PopUpMenu popUpMenu_;
    FirmwareListMenu firmwareListMenu_;
    OtaStatusMenu otaStatusMenu_;
};

#endif