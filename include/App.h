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
#include "TextInputMenu.h"
#include "ConnectionStatusMenu.h"
#include "PopUpMenu.h"
#include "OtaManager.h"
#include "OtaStatusMenu.h"
#include "SplitSelectionMenu.h"
#include "Jammer.h"
#include "ChannelSelectionMenu.h"
#include "JammingActiveMenu.h"
#include "ListMenu.h"
#include "WifiListDataSource.h"
#include "FirmwareListDataSource.h"
#include "BeaconSpammer.h"
#include "BeaconFileListDataSource.h"
#include "BeaconSpamActiveMenu.h"
#include "Deauther.h"
#include "DeauthActiveMenu.h"
#include "EvilTwin.h"
#include "EvilTwinActiveMenu.h"
#include "PortalListDataSource.h"
#include "Logger.h"

class App
{
public:
    App();
    void setup();
    void loop();

    void changeMenu(MenuType type, bool isForwardNav = true);
    void returnToMenu(MenuType type);

    void showPopUp(std::string title, std::string message, PopUpMenu::OnConfirmCallback onConfirm,
                   const std::string &confirmText = "OK", const std::string &cancelText = "Cancel", bool executeOnConfirmBeforeExit = false);
    HardwareManager &getHardwareManager() { return hardware_; }
    WifiManager &getWifiManager() { return wifiManager_; }
    OtaManager &getOtaManager() { return otaManager_; }
    Jammer &getJammer() { return jammer_; }
    BeaconSpammer &getBeaconSpammer() { return beaconSpammer_; }
    Deauther &getDeauther() { return deauther_; }
    EvilTwin &getEvilTwin() { return evilTwin_; }
    TextInputMenu &getTextInputMenu() { return textInputMenu_; }
    IMenu *getMenu(MenuType type);
    WifiListDataSource &getWifiListDataSource() { return wifiListDataSource_; }

    MenuType getPreviousMenuType() const;

    void drawStatusBar();

private:
    void drawSecondaryDisplay();

    void updateAndDrawBootScreen(unsigned long bootStartTime, unsigned long totalBootDuration);
    void logToSmallDisplay(const char *message, const char *status = nullptr);

    float currentProgressBarFillPx_;

    static const int MAX_LOG_LINES_SMALL_DISPLAY = 4;
    static const int MAX_LOG_LINE_LENGTH_SMALL_DISPLAY = 32;
    char smallDisplayLogBuffer_[MAX_LOG_LINES_SMALL_DISPLAY][MAX_LOG_LINE_LENGTH_SMALL_DISPLAY];

    HardwareManager hardware_;
    WifiManager wifiManager_;
    OtaManager otaManager_;
    Jammer jammer_;
    BeaconSpammer beaconSpammer_;
    Deauther deauther_;
    EvilTwin evilTwin_;

    std::map<MenuType, IMenu *> menuRegistry_;
    IMenu *currentMenu_;

    std::vector<MenuType> navigationStack_;

    // Concrete Menu Instances
    MainMenu mainMenu_;

    CarouselMenu toolsMenu_;
    CarouselMenu gamesMenu_;
    CarouselMenu settingsMenu_;
    CarouselMenu utilitiesMenu_;

    GridMenu wifiToolsMenu_;
    GridMenu firmwareUpdateGrid_;
    GridMenu jammingToolsMenu_;
    GridMenu deauthToolsMenu_;
    
    SplitSelectionMenu beaconModeMenu_;

    // WifiListMenu wifiListMenu_;
    TextInputMenu textInputMenu_;
    ConnectionStatusMenu connectionStatusMenu_;
    PopUpMenu popUpMenu_;

    // FirmwareListMenu firmwareListMenu_;
    OtaStatusMenu otaStatusMenu_;
    ChannelSelectionMenu channelSelectionMenu_;
    
    JammingActiveMenu jammingActiveMenu_;
    BeaconSpamActiveMenu beaconSpamActiveMenu_;
    DeauthActiveMenu deauthActiveMenu_;
    EvilTwinActiveMenu evilTwinActiveMenu_;

    // --- NEW GENERIC LIST MENU SYSTEM ---
    WifiListDataSource wifiListDataSource_;
    FirmwareListDataSource firmwareListDataSource_;
    BeaconFileListDataSource beaconFileListDataSource_;
    PortalListDataSource portalListDataSource_;
    ListMenu wifiListMenu_;
    ListMenu firmwareListMenu_;
    ListMenu beaconFileListMenu_;
    ListMenu portalListMenu_;
};

#endif