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
#include "EvilPortal.h"
#include "EvilPortalActiveMenu.h"
#include "PortalListDataSource.h"
#include "ProbeSniffer.h"
#include "ProbeSnifferActiveMenu.h"
#include "KarmaAttacker.h"
#include "KarmaActiveMenu.h"
#include "HandshakeCapture.h"
#include "HandshakeCaptureMenu.h"
#include "HandshakeCaptureActiveMenu.h"
#include "ProbeFlooder.h"
#include "ProbeFloodActiveMenu.h"
#include "BleSpammer.h"
#include "BleSpamActiveMenu.h"
#include "DuckyScriptRunner.h"
#include "DuckyScriptActiveMenu.h"
#include "DuckyScriptListDataSource.h"
#include "BrightnessMenu.h"
#include "ConfigManager.h"
#include "UsbDriveMenu.h"
#include "MusicPlayer.h"
#include "MusicLibraryDataSource.h"
#include "SongListDataSource.h"
#include "NowPlayingMenu.h"
#include "MusicLibraryManager.h"
#include "Logger.h"
#include <HIDForge.h>
#include "InfoMenu.h" 
#include "ActionListDataSource.h"
#include "SnakeGameMenu.h"
#include "GameAudio.h"
#include "EventDispatcher.h"
#include "StationSniffer.h"
#include "AssociationSleeper.h"
#include "AssociationSleepActiveMenu.h"
#include "StationListDataSource.h"


class App : public ISubscriber
{
public:
    static App& getInstance();

    // Deleted copy constructor and assignment operator
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    void setup();
    void loop();

    void onEvent(const Event& event) override;

    void showPopUp(std::string title, std::string message, PopUpMenu::OnConfirmCallback onConfirm,
                   const std::string &confirmText = "OK", const std::string &cancelText = "Cancel", bool executeOnConfirmBeforeExit = false);
    HardwareManager &getHardwareManager() { return hardware_; }
    WifiManager &getWifiManager() { return wifiManager_; }
    OtaManager &getOtaManager() { return otaManager_; }
    Jammer &getJammer() { return jammer_; }
    BeaconSpammer &getBeaconSpammer() { return beaconSpammer_; }
    Deauther &getDeauther() { return deauther_; }
    EvilPortal &getEvilPortal() { return evilPortal_; }
    ProbeSniffer &getProbeSniffer() { return probeSniffer_; }
    KarmaAttacker &getKarmaAttacker() { return karmaAttacker_; }
    HandshakeCapture &getHandshakeCapture() { return handshakeCapture_; }
    ProbeFlooder &getProbeFlooder() { return probeFlooder_; }
    BleSpammer &getBleSpammer() { return bleSpammer_; }
    DuckyScriptRunner &getDuckyRunner() { return duckyRunner_; }
    BleManager &getBleManager() { return bleManager_; } 
    ConfigManager &getConfigManager() { return configManager_; }
    MusicPlayer &getMusicPlayer() { return musicPlayer_; }
    MusicLibraryManager &getMusicLibraryManager() { return musicLibraryManager_; }
    MusicLibraryDataSource& getMusicLibraryDataSource() { return musicLibraryDataSource_; }
    SongListDataSource& getSongListDataSource() { return songListDataSource_; }
    GameAudio &getGameAudio() { return gameAudio_; }
    StationSniffer &getStationSniffer() { return stationSniffer_; }
    AssociationSleeper &getAssociationSleeper() { return associationSleeper_; }
    DuckyScriptListDataSource &getDuckyScriptListDataSource() { return duckyScriptListDataSource_; }
    TextInputMenu &getTextInputMenu() { return textInputMenu_; }
    IMenu *getMenu(MenuType type);
    WifiListDataSource &getWifiListDataSource() { return wifiListDataSource_; }
    StationListDataSource &getStationListDataSource() { return stationListDataSource_; }

    MenuType getPreviousMenuType() const;

    void drawStatusBar();

private:
    App(); // Private constructor

    void changeMenu(MenuType type, bool isForwardNav = true);
    void replaceMenu(MenuType type);
    void returnToMenu(MenuType type);

    void drawSecondaryDisplay();

    void updateAndDrawBootScreen(unsigned long bootStartTime, unsigned long totalBootDuration);
    void logToSmallDisplay(const char *message, const char *status = nullptr);

    // --- MODIFICATION START: Add pending navigation state variables ---
    MenuType pendingMenuChange_{MenuType::NONE};
    MenuType pendingReturnMenu_{MenuType::NONE};
    MenuType pendingReplaceMenu_{MenuType::NONE};
    bool isForwardNavPending_{true};
    bool backNavPending_{false};
    // --- MODIFICATION END ---

    float currentProgressBarFillPx_;

    // --- NEW: Marquee state for the status bar title ---
    char statusBarMarqueeText_[40];
    int statusBarMarqueeTextLenPx_;
    float statusBarMarqueeOffset_;
    unsigned long lastStatusBarMarqueeTime_;
    bool statusBarMarqueeActive_;
    bool statusBarMarqueePaused_;
    unsigned long statusBarMarqueePauseStartTime_;
    bool statusBarMarqueeScrollLeft_;

    static const int MAX_LOG_LINES_SMALL_DISPLAY = 4;
    static const int MAX_LOG_LINE_LENGTH_SMALL_DISPLAY = 32;
    char smallDisplayLogBuffer_[MAX_LOG_LINES_SMALL_DISPLAY][MAX_LOG_LINE_LENGTH_SMALL_DISPLAY];

    HardwareManager hardware_;
    WifiManager wifiManager_;
    OtaManager otaManager_;
    Jammer jammer_;
    BeaconSpammer beaconSpammer_;
    Deauther deauther_;
    EvilPortal evilPortal_;
    ProbeSniffer probeSniffer_;
    KarmaAttacker karmaAttacker_;
    HandshakeCapture handshakeCapture_;
    ProbeFlooder probeFlooder_;
    BleSpammer bleSpammer_;
    DuckyScriptRunner duckyRunner_;
    BleManager bleManager_;
    ConfigManager configManager_;
    MusicPlayer musicPlayer_;
    MusicLibraryManager musicLibraryManager_;
    GameAudio gameAudio_;
    StationSniffer stationSniffer_;
    AssociationSleeper associationSleeper_;

    std::map<MenuType, IMenu *> menuRegistry_;
    IMenu *currentMenu_;

    std::vector<MenuType> navigationStack_;

    // Concrete Menu Instances
    MainMenu mainMenu_;
    CarouselMenu toolsMenu_;
    CarouselMenu gamesMenu_;
    UsbDriveMenu usbDriveMenu_;
    InfoMenu infoMenu_;
    SnakeGameMenu snakeGameMenu_;

    // New Menu Structure
    GridMenu wifiToolsMenu_;
    GridMenu bleToolsMenu_;
    GridMenu nrfJammerMenu_;
    GridMenu hostOtherMenu_;
    GridMenu beaconModeMenu_;
    GridMenu deauthModeMenu_;
    GridMenu probeFloodModeMenu_;
    GridMenu associationSleepModeMenu_;
    GridMenu settingsGridMenu_;
    GridMenu firmwareUpdateGrid_;

    BrightnessMenu brightnessMenu_;
    TextInputMenu textInputMenu_;
    ConnectionStatusMenu connectionStatusMenu_;
    PopUpMenu popUpMenu_;
    OtaStatusMenu otaStatusMenu_;
    ChannelSelectionMenu channelSelectionMenu_;

    // Active Screens
    JammingActiveMenu jammingActiveMenu_;
    BeaconSpamActiveMenu beaconSpamActiveMenu_;
    DeauthActiveMenu deauthActiveMenu_;
    EvilPortalActiveMenu evilPortalActiveMenu_;
    ProbeSnifferActiveMenu probeSnifferActiveMenu_;
    KarmaActiveMenu karmaActiveMenu_;
    HandshakeCaptureMenu handshakeCaptureMenu_;
    ProbeFloodActiveMenu probeFloodActiveMenu_;
    HandshakeCaptureActiveMenu handshakeCaptureActiveMenu_;
    BleSpamActiveMenu bleSpamActiveMenu_;
    DuckyScriptActiveMenu duckyScriptActiveMenu_;
    NowPlayingMenu nowPlayingMenu_;
    AssociationSleepActiveMenu associationSleepActiveMenu_;

    // DataSources
    WifiListDataSource wifiListDataSource_;
    StationListDataSource stationListDataSource_;
    FirmwareListDataSource firmwareListDataSource_;
    BeaconFileListDataSource beaconFileListDataSource_;
    PortalListDataSource portalListDataSource_;
    DuckyScriptListDataSource duckyScriptListDataSource_;
    MusicLibraryDataSource musicLibraryDataSource_;
    SongListDataSource songListDataSource_;
    
    // New Generic DataSources
    ActionListDataSource wifiAttacksDataSource_;
    ActionListDataSource wifiSniffDataSource_;
    ActionListDataSource bleAttacksDataSource_;
    ActionListDataSource uiSettingsDataSource_;
    ActionListDataSource hardwareSettingsDataSource_;
    ActionListDataSource connectivitySettingsDataSource_;
    ActionListDataSource systemSettingsDataSource_;
    // --- NEW GAME DATASOURCES ---
    ActionListDataSource snakeMenuDataSource_;
    ActionListDataSource tetrisMenuDataSource_;

    // ListMenu Instances
    ListMenu wifiListMenu_;
    ListMenu stationListMenu_;
    ListMenu firmwareListMenu_;
    ListMenu beaconFileListMenu_;
    ListMenu portalListMenu_;
    ListMenu duckyScriptListMenu_;
    ListMenu musicLibraryMenu_;
    ListMenu songListMenu_;
    
    // New ListMenus using the generic source
    ListMenu wifiAttacksMenu_;
    ListMenu wifiSniffMenu_;
    ListMenu bleAttacksMenu_;
    ListMenu uiSettingsMenu_;
    ListMenu hardwareSettingsMenu_;
    ListMenu connectivitySettingsMenu_;
    ListMenu systemSettingsMenu_;

    // --- GAME LISTMENUS ---
    ListMenu snakeMenu_;
    ListMenu tetrisMenu_;
};

#endif