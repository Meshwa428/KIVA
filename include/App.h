#ifndef APP_H
#define APP_H

#include "HardwareManager.h"
#include "IMenu.h"
#include "MainMenu.h"
#include "CarouselMenu.h"
#include "GridMenu.h"
#include <vector>
#include <map>
#include <functional>
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
#include "ProbeSniffer.h"
#include "ProbeSnifferActiveMenu.h"
#include "KarmaAttacker.h"
#include "KarmaActiveMenu.h"
#include "HandshakeCapture.h"
#include "HandshakeCaptureMenu.h"
#include "HandshakeCaptureActiveMenu.h"
#include "BleSpammer.h"
#include "BleSpamActiveMenu.h"
#include "DuckyScriptRunner.h"
#include "DuckyScriptActiveMenu.h"
#include "DuckyScriptListDataSource.h"
#include "SettingsMenu.h"
#include "BrightnessMenu.h"
#include "ConfigManager.h"
#include "UsbDriveMenu.h"
#include "MusicPlayer.h"
#include "MusicPlayListDataSource.h"
#include "NowPlayingMenu.h"
#include "MusicLibraryManager.h"
#include "Logger.h"
#include <HIDForge.h> // <-- ADD THIS

class App
{
public:
    App();
    void setup();
    void loop();

    void changeMenu(MenuType type, bool isForwardNav = true);
    void replaceMenu(MenuType type);
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
    ProbeSniffer &getProbeSniffer() { return probeSniffer_; }
    KarmaAttacker &getKarmaAttacker() { return karmaAttacker_; }
    HandshakeCapture &getHandshakeCapture() { return handshakeCapture_; }
    BleSpammer &getBleSpammer() { return bleSpammer_; }
    DuckyScriptRunner &getDuckyRunner() { return duckyRunner_; }
    BleManager &getBleManager() { return bleManager_; } // <-- Now returns HIDForge::BleManager
    ConfigManager &getConfigManager() { return configManager_; }
    MusicPlayer &getMusicPlayer() { return musicPlayer_; }
    MusicLibraryManager &getMusicLibraryManager() { return musicLibraryManager_; }
    DuckyScriptListDataSource &getDuckyScriptListDataSource() { return duckyScriptListDataSource_; }
    TextInputMenu &getTextInputMenu() { return textInputMenu_; }
    IMenu *getMenu(MenuType type);
    WifiListDataSource &getWifiListDataSource() { return wifiListDataSource_; }

    MenuType getPreviousMenuType() const;
    void clearInputQueue() { hardware_.clearInputQueue(); }

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
    ProbeSniffer probeSniffer_;
    KarmaAttacker karmaAttacker_;
    HandshakeCapture handshakeCapture_;
    BleSpammer bleSpammer_;
    DuckyScriptRunner duckyRunner_;
    BleManager bleManager_; // <-- Now using HIDForge::BleManager
    ConfigManager configManager_;
    MusicPlayer musicPlayer_;
    MusicLibraryManager musicLibraryManager_;

    std::map<MenuType, IMenu *> menuRegistry_;
    IMenu *currentMenu_;

    std::vector<MenuType> navigationStack_;

    // Concrete Menu Instances
    MainMenu mainMenu_;

    CarouselMenu toolsMenu_;
    CarouselMenu gamesMenu_;
    CarouselMenu utilitiesMenu_;
    UsbDriveMenu usbDriveMenu_;

    GridMenu wifiToolsMenu_;
    GridMenu bleToolsMenu_;
    GridMenu hostToolsMenu_;
    GridMenu firmwareUpdateGrid_;
    GridMenu jammingToolsMenu_;
    GridMenu deauthToolsMenu_;

    SettingsMenu settingsMenu_;
    BrightnessMenu brightnessMenu_;

    SplitSelectionMenu beaconModeMenu_;

    TextInputMenu textInputMenu_;
    ConnectionStatusMenu connectionStatusMenu_;
    PopUpMenu popUpMenu_;

    OtaStatusMenu otaStatusMenu_;
    ChannelSelectionMenu channelSelectionMenu_;

    JammingActiveMenu jammingActiveMenu_;
    BeaconSpamActiveMenu beaconSpamActiveMenu_;
    DeauthActiveMenu deauthActiveMenu_;
    EvilTwinActiveMenu evilTwinActiveMenu_;
    ProbeSnifferActiveMenu probeSnifferActiveMenu_;
    KarmaActiveMenu karmaActiveMenu_;
    HandshakeCaptureMenu handshakeCaptureMenu_;
    HandshakeCaptureActiveMenu handshakeCaptureActiveMenu_;
    BleSpamActiveMenu bleSpamActiveMenu_;
    DuckyScriptActiveMenu duckyScriptActiveMenu_;

    WifiListDataSource wifiListDataSource_;
    FirmwareListDataSource firmwareListDataSource_;
    BeaconFileListDataSource beaconFileListDataSource_;
    PortalListDataSource portalListDataSource_;
    DuckyScriptListDataSource duckyScriptListDataSource_;

    ListMenu wifiListMenu_;
    ListMenu firmwareListMenu_;
    ListMenu beaconFileListMenu_;
    ListMenu portalListMenu_;
    ListMenu duckyScriptListMenu_;

    ListMenu musicPlayerListMenu_;
    NowPlayingMenu nowPlayingMenu_;

    MusicPlayListDataSource musicPlayListDataSource_;
};

#endif