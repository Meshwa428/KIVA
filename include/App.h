#ifndef APP_H
#define APP_H

#include "BeaconFileListDataSource.h"
#include "IMenu.h"
#include "MainMenu.h"
#include "CarouselMenu.h"
#include "GridMenu.h"
#include <vector>
#include <map>
#include <functional>
#include "TextInputMenu.h"
#include "ConnectionStatusMenu.h"
#include "PopUpMenu.h"
#include "OtaStatusMenu.h"
#include "ChannelSelectionMenu.h"
#include "JammingActiveMenu.h"
#include "ListMenu.h"
#include "WifiListDataSource.h"
#include "FirmwareListDataSource.h"
#include "BeaconSpamActiveMenu.h"
#include "DeauthActiveMenu.h"
#include "EvilPortalActiveMenu.h"
#include "PortalListDataSource.h"
#include "ProbeSnifferActiveMenu.h"
#include "KarmaActiveMenu.h"
#include "HandshakeCaptureMenu.h"
#include "HandshakeCaptureActiveMenu.h"
#include "ProbeFloodActiveMenu.h"
#include "BleSpamActiveMenu.h"
#include "DuckyScriptActiveMenu.h"
#include "DuckyScriptListDataSource.h"
#include "BrightnessMenu.h"
#include "UsbDriveMenu.h"
#include "MusicLibraryDataSource.h"
#include "SongListDataSource.h"
#include "NowPlayingMenu.h"
#include "Logger.h"
#include <HIDForge.h>
#include "InfoMenu.h" 
#include "ActionListDataSource.h"
#include "SnakeGameMenu.h"
#include "EventDispatcher.h"
#include "AssociationSleepActiveMenu.h"
#include "StationListDataSource.h"
#include "TimezoneListDataSource.h"
#include "ServiceManager.h"
#include "MyBleManagerService.h"
#include <memory>

class HardwareManager;
class WifiManager;
class OtaManager;
class Jammer;
class BeaconSpammer;
class Deauther;
class EvilPortal;
class ProbeSniffer;
class KarmaAttacker;
class HandshakeCapture;
class ProbeFlooder;
class BleSpammer;
class DuckyScriptRunner;

class ConfigManager;
class MusicPlayer;
class MusicLibraryManager;
class GameAudio;
class StationSniffer;
class AssociationSleeper;
class RtcManager;
class SystemDataProvider;

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
    
    HardwareManager &getHardwareManager();
    WifiManager &getWifiManager();
    OtaManager &getOtaManager();
    Jammer &getJammer();
    BeaconSpammer &getBeaconSpammer();
    Deauther &getDeauther();
    EvilPortal &getEvilPortal();
    ProbeSniffer &getProbeSniffer();
    KarmaAttacker &getKarmaAttacker();
    HandshakeCapture &getHandshakeCapture();
    ProbeFlooder &getProbeFlooder();
    BleSpammer &getBleSpammer();
    DuckyScriptRunner &getDuckyRunner();
    MyBleManagerService &getBleManager();
    ConfigManager &getConfigManager();
    MusicPlayer &getMusicPlayer();
    MusicLibraryManager &getMusicLibraryManager();
    MusicLibraryDataSource& getMusicLibraryDataSource();
    SongListDataSource& getSongListDataSource();
    GameAudio &getGameAudio();
    StationSniffer &getStationSniffer();
    AssociationSleeper &getAssociationSleeper();
    RtcManager& getRtcManager();
    TimezoneListDataSource& getTimezoneListDataSource();
    SystemDataProvider& getSystemDataProvider();

    const ConfigManager &getConfigManager() const;
    const HardwareManager &getHardwareManager() const;
    const RtcManager &getRtcManager() const;
    const SystemDataProvider &getSystemDataProvider() const;
    const WifiManager &getWifiManager() const;

    void toggleSecondaryWidget(SecondaryWidgetType type);
    bool isSecondaryWidgetActive(SecondaryWidgetType type) const;
    std::vector<SecondaryWidgetType> getActiveSecondaryWidgets() const;
    DuckyScriptListDataSource &getDuckyScriptListDataSource() { return duckyScriptListDataSource_; }
    TextInputMenu &getTextInputMenu() { return textInputMenu_; }
    IMenu *getMenu(MenuType type);
    WifiListDataSource &getWifiListDataSource() { return wifiListDataSource_; }
    StationListDataSource &getStationListDataSource() { return stationListDataSource_; }

    MenuType getPreviousMenuType() const;

    void drawStatusBar();
    void requestRedraw();

private:
    App(); // Private constructor

    void changeMenu(MenuType type, bool isForwardNav = true);
    void replaceMenu(MenuType type);
    void returnToMenu(MenuType type);

    void drawSecondaryDisplay();

    void updateAndDrawBootScreen(unsigned long bootStartTime, unsigned long totalBootDuration);
    void logToSmallDisplay(const char *message, const char *status = nullptr);

    // --- NEW: Redraw optimization state ---
    bool redrawRequested_ = true;
    unsigned long lastDrawTime_ = 0;

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

    std::unique_ptr<ServiceManager> serviceManager_;

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
    TimezoneListDataSource timezoneDataSource_;
    
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
    ListMenu timezoneMenu_;

    // --- GAME LISTMENUS ---
    ListMenu snakeMenu_;
    ListMenu tetrisMenu_;
};

#endif