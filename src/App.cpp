#include "App.h"
#include "Icons.h"
#include "UI_Utils.h"
#include "Jammer.h"
#include "BleSpammer.h"
#include "DuckyScriptRunner.h"
#include "DebugUtils.h"
#include <algorithm>
#include <cmath>
#include <SdCardManager.h>
#include <esp_task_wdt.h>

App::App() : 
    currentMenu_(nullptr),
    currentProgressBarFillPx_(0.0f),
    mainMenu_(),
    toolsMenu_("Tools", {
        MenuItem{"WiFi Tools", IconType::NET_WIFI, MenuType::WIFI_TOOLS_GRID},
        MenuItem{"BLE Tools", IconType::NET_BLUETOOTH, MenuType::BLE_TOOLS_GRID},
        MenuItem{"Host Tools", IconType::TOOL_INJECTION, MenuType::HOST_TOOLS_GRID},
        MenuItem{"Jamming", IconType::TOOL_JAMMING, MenuType::JAMMING_TOOLS_GRID},
        MenuItem{"Back", IconType::NAV_BACK, MenuType::BACK}}),
    gamesMenu_("Games", {
        MenuItem{"Snake", IconType::GAME_SNAKE, MenuType::NONE},
        MenuItem{"Tetris", IconType::GAME_TETRIS, MenuType::NONE},
        MenuItem{"Back", IconType::NAV_BACK, MenuType::BACK}}),
    utilitiesMenu_("Utilities", {
        MenuItem{"Vibration", IconType::UI_VIBRATION, MenuType::NONE,
            [](App *app) {
                HardwareManager &hw = app->getHardwareManager();
                hw.setVibration(!hw.isVibrationOn());
            }
        },
        MenuItem{"Laser", IconType::UI_LASER, MenuType::NONE,
                [](App *app)
                {
                    HardwareManager &hw = app->getHardwareManager();
                    hw.setLaser(!hw.isLaserOn());
                }},
        // --- START OF FIX ---
        MenuItem{"USB Drive Mode", IconType::SD_CARD, MenuType::USB_DRIVE_MODE},
        // --- END OF FIX ---
        MenuItem{"Back", IconType::NAV_BACK, MenuType::BACK}
    }),
    settingsMenu_(),
    brightnessMenu_(),
    wifiToolsMenu_("WiFi Tools", {
        MenuItem{"Beacon Spam", IconType::TOOL_JAMMING, MenuType::BEACON_MODE_SELECTION},
        MenuItem{"Deauth", IconType::DISCONNECT, MenuType::DEAUTH_TOOLS_GRID},
        MenuItem{"Evil Twin", IconType::SKULL, MenuType::EVIL_TWIN_PORTAL_SELECTION},
        MenuItem{"Probe Sniff", IconType::UI_REFRESH, MenuType::PROBE_ACTIVE},
        MenuItem{"Karma Attack", IconType::TOOL_INJECTION, MenuType::KARMA_ACTIVE},
        MenuItem{"Handshake Sniffer", IconType::WIFI_SCAN, MenuType::HANDSHAKE_CAPTURE_MENU},
        MenuItem{"Back", IconType::NAV_BACK, MenuType::BACK}
    }, 2),
    bleToolsMenu_("BLE Tools", {
        MenuItem{"Apple Juice", IconType::BEACON, MenuType::NONE, 
            [](App* app){
                auto* menu = static_cast<BleSpamActiveMenu*>(app->getMenu(MenuType::BLE_SPAM_ACTIVE));
                if(menu) { menu->setSpamModeToStart(BleSpamMode::APPLE_JUICE); app->changeMenu(MenuType::BLE_SPAM_ACTIVE); }
            }},
        MenuItem{"Sour Apple", IconType::BEACON, MenuType::NONE, 
            [](App* app){
                auto* menu = static_cast<BleSpamActiveMenu*>(app->getMenu(MenuType::BLE_SPAM_ACTIVE));
                if(menu) { menu->setSpamModeToStart(BleSpamMode::SOUR_APPLE); app->changeMenu(MenuType::BLE_SPAM_ACTIVE); }
            }},
        MenuItem{"Android", IconType::BEACON, MenuType::NONE,
            [](App* app){
                auto* menu = static_cast<BleSpamActiveMenu*>(app->getMenu(MenuType::BLE_SPAM_ACTIVE));
                if(menu) { menu->setSpamModeToStart(BleSpamMode::GOOGLE); app->changeMenu(MenuType::BLE_SPAM_ACTIVE); }
            }},
        MenuItem{"Samsung", IconType::BEACON, MenuType::NONE,
            [](App* app){
                auto* menu = static_cast<BleSpamActiveMenu*>(app->getMenu(MenuType::BLE_SPAM_ACTIVE));
                if(menu) { menu->setSpamModeToStart(BleSpamMode::SAMSUNG); app->changeMenu(MenuType::BLE_SPAM_ACTIVE); }
            }},
        MenuItem{"Swift Pair", IconType::BEACON, MenuType::NONE,
            [](App* app){
                auto* menu = static_cast<BleSpamActiveMenu*>(app->getMenu(MenuType::BLE_SPAM_ACTIVE));
                if(menu) { menu->setSpamModeToStart(BleSpamMode::MICROSOFT); app->changeMenu(MenuType::BLE_SPAM_ACTIVE); }
            }},
        MenuItem{"Tutti-Frutti", IconType::SKULL, MenuType::NONE,
            [](App* app){
                auto* menu = static_cast<BleSpamActiveMenu*>(app->getMenu(MenuType::BLE_SPAM_ACTIVE));
                if(menu) { menu->setSpamModeToStart(BleSpamMode::ALL); app->changeMenu(MenuType::BLE_SPAM_ACTIVE); }
            }},
        MenuItem{"Back", IconType::NAV_BACK, MenuType::BACK}
    }, 2),
    hostToolsMenu_("Host Tools", {
        MenuItem{"USB Ducky", IconType::USB, MenuType::NONE, 
            [](App* app){
                app->getDuckyScriptListDataSource().setExecutionMode(DuckyScriptRunner::Mode::USB);
                app->changeMenu(MenuType::DUCKY_SCRIPT_LIST);
            }},
        MenuItem{"BLE Ducky", IconType::NET_BLUETOOTH, MenuType::NONE, 
            [](App* app){
                app->getDuckyScriptListDataSource().setExecutionMode(DuckyScriptRunner::Mode::BLE);
                app->changeMenu(MenuType::DUCKY_SCRIPT_LIST);
            }},
        MenuItem{"BLE Keyboard", IconType::NONE, MenuType::NONE},
        MenuItem{"BLE Media", IconType::NONE, MenuType::NONE},
        MenuItem{"BLE Scroller", IconType::NONE, MenuType::NONE},
        MenuItem{"Back", IconType::NAV_BACK, MenuType::BACK}
    }, 2),
    firmwareUpdateGrid_("Update", {
        MenuItem{"Web Update", IconType::FIRMWARE_UPDATE, MenuType::NONE,
            [](App *app) {
                if (app->getOtaManager().startWebUpdate()) {
                    app->changeMenu(MenuType::OTA_STATUS);
                }
                else {
                    app->showPopUp("Error", "Failed to start Web AP.", nullptr, "OK", "", true);
                }
            }
        },
        MenuItem{"SD Card", IconType::SD_CARD, MenuType::FIRMWARE_LIST_SD}, 
        MenuItem{"Basic OTA", IconType::BASIC_OTA, MenuType::NONE,
            [](App *app) {
                app->getOtaManager().startBasicOta();
            }
        },
        MenuItem{"Back", IconType::NAV_BACK, MenuType::BACK}
    }, 2),
    jammingToolsMenu_("Jammer", {
        MenuItem{"BLE", IconType::NET_BLUETOOTH, MenuType::NONE,
            [](App *app) {
                JammingActiveMenu *jammerMenu = static_cast<JammingActiveMenu *>(app->getMenu(MenuType::JAMMING_ACTIVE));
                if (jammerMenu)
                {
                    jammerMenu->setJammingModeToStart(JammingMode::BLE);
                    JammerConfig cfg;
                    cfg.technique = JammingTechnique::NOISE_INJECTION;
                    jammerMenu->setJammingConfig(cfg);
                    app->changeMenu(MenuType::JAMMING_ACTIVE);
                }
            }},
        MenuItem{"BT Classic", IconType::NET_BLUETOOTH, MenuType::NONE,
            [](App *app) {
                JammingActiveMenu *jammerMenu = static_cast<JammingActiveMenu *>(app->getMenu(MenuType::JAMMING_ACTIVE));
                if (jammerMenu) {
                    jammerMenu->setJammingModeToStart(JammingMode::BT_CLASSIC);
                    JammerConfig cfg;
                    cfg.technique = JammingTechnique::CONSTANT_CARRIER;
                    jammerMenu->setJammingConfig(cfg);
                    app->changeMenu(MenuType::JAMMING_ACTIVE);
                }
            }},
        MenuItem{"WiFi Narrow", IconType::NET_WIFI, MenuType::NONE,
            [](App *app) {
                JammingActiveMenu *jammerMenu = static_cast<JammingActiveMenu *>(app->getMenu(MenuType::JAMMING_ACTIVE));
                if (jammerMenu) {
                    jammerMenu->setJammingModeToStart(JammingMode::WIFI_NARROWBAND);
                    JammerConfig cfg;
                    cfg.technique = JammingTechnique::NOISE_INJECTION;
                    jammerMenu->setJammingConfig(cfg);
                    app->changeMenu(MenuType::JAMMING_ACTIVE);
                }
            }},
        MenuItem{"Wide Spectrum", IconType::TOOL_JAMMING, MenuType::NONE,
            [](App *app) {
                JammingActiveMenu *jammerMenu = static_cast<JammingActiveMenu *>(app->getMenu(MenuType::JAMMING_ACTIVE));
                if (jammerMenu)
                {
                    jammerMenu->setJammingModeToStart(JammingMode::WIDE_SPECTRUM);
                    JammerConfig cfg;
                    cfg.technique = JammingTechnique::CONSTANT_CARRIER;
                    jammerMenu->setJammingConfig(cfg);
                    app->changeMenu(MenuType::JAMMING_ACTIVE);
                }
            }},
        MenuItem{"Zigbee", IconType::TOOL_INJECTION, MenuType::NONE, [](App *app)
                {
                    JammingActiveMenu *jammerMenu = static_cast<JammingActiveMenu *>(app->getMenu(MenuType::JAMMING_ACTIVE));
                    if (jammerMenu)
                    {
                        jammerMenu->setJammingModeToStart(JammingMode::ZIGBEE);
                        JammerConfig cfg;
                        cfg.technique = JammingTechnique::NOISE_INJECTION;
                        jammerMenu->setJammingConfig(cfg);
                        app->changeMenu(MenuType::JAMMING_ACTIVE);
                    }
                }},
        MenuItem{"Custom Flood", IconType::TOOL_INJECTION, MenuType::CHANNEL_SELECTION},
        MenuItem{"Back", IconType::NAV_BACK, MenuType::BACK}},2),
    deauthToolsMenu_("Deauth Attack", {
        MenuItem{"RogueAP (Once)", IconType::BEACON, MenuType::NONE,
            [](App *app) {
                app->getDeauther().prepareAttack(DeauthMode::ROGUE_AP, DeauthTarget::SPECIFIC_AP);
                app->getWifiListDataSource().setScanOnEnter(true);
                app->changeMenu(MenuType::WIFI_LIST);
            }
        },
        MenuItem{"Bcast (Once)", IconType::SKULL, MenuType::NONE, 
            [](App *app) {
                app->getDeauther().prepareAttack(DeauthMode::BROADCAST, DeauthTarget::SPECIFIC_AP);
                app->getWifiListDataSource().setScanOnEnter(true);
                app->changeMenu(MenuType::WIFI_LIST);
            }},
        MenuItem{"RogueAP (All)", IconType::BEACON, MenuType::NONE, 
            [](App *app) {
                app->getDeauther().prepareAttack(DeauthMode::ROGUE_AP, DeauthTarget::ALL_APS);
                app->changeMenu(MenuType::DEAUTH_ACTIVE);
            }},
        MenuItem{"Bcast (All)", IconType::SKULL, MenuType::NONE, 
            [](App *app) {
                app->getDeauther().prepareAttack(DeauthMode::BROADCAST, DeauthTarget::ALL_APS);
                app->changeMenu(MenuType::DEAUTH_ACTIVE);
            }},
        MenuItem{"Back", IconType::NAV_BACK, MenuType::BACK}},2),
    beaconModeMenu_("Beacon Spam", MenuType::BEACON_MODE_SELECTION, {
        MenuItem{"Random", IconType::BEACON, MenuType::NONE, 
            [](App *app) {
                BeaconSpamActiveMenu *activeMenu = static_cast<BeaconSpamActiveMenu *>(app->getMenu(MenuType::BEACON_SPAM_ACTIVE));
                if (activeMenu)
                {
                    activeMenu->setAttackParameters(BeaconSsidMode::RANDOM);
                    app->changeMenu(MenuType::BEACON_SPAM_ACTIVE);
                }
            }
        },
        MenuItem{"From SD", IconType::SD_CARD, MenuType::BEACON_FILE_LIST}, 
        MenuItem{"Back", IconType::NAV_BACK, MenuType::BACK}
    }),
    textInputMenu_(), 
    connectionStatusMenu_(),
    popUpMenu_(),
    otaStatusMenu_(),
    channelSelectionMenu_(),
    jammingActiveMenu_(),
    jammer_(),
    beaconSpammer_(),
    karmaAttacker_(),
    handshakeCapture_(),
    wifiListDataSource_(),
    firmwareListDataSource_(),
    beaconFileListDataSource_(),
    wifiListMenu_("Wi-Fi Setup", MenuType::WIFI_LIST, &wifiListDataSource_),
    firmwareListMenu_("Update from SD", MenuType::FIRMWARE_LIST_SD, &firmwareListDataSource_),
    beaconFileListMenu_("Select SSID File", MenuType::BEACON_FILE_LIST, &beaconFileListDataSource_),
    portalListMenu_("Select Portal", MenuType::EVIL_TWIN_PORTAL_SELECTION, &portalListDataSource_),
    duckyScriptListMenu_("Ducky Scripts", MenuType::DUCKY_SCRIPT_LIST, &duckyScriptListDataSource_),
    beaconSpamActiveMenu_(),
    deauthActiveMenu_(),
    evilTwinActiveMenu_(),
    probeSnifferActiveMenu_(),
    karmaActiveMenu_(),
    handshakeCaptureMenu_(),
    handshakeCaptureActiveMenu_(),
    bleSpamActiveMenu_(),
    duckyRunner_(),
    bleManager_(),
    duckyScriptListDataSource_(),
    duckyScriptActiveMenu_(),
    musicPlayerListMenu_("Music Player", MenuType::MUSIC_PLAYER_LIST, &musicPlayListDataSource_),
    nowPlayingMenu_() 
{
    for (int i = 0; i < MAX_LOG_LINES_SMALL_DISPLAY; ++i)
    {
        smallDisplayLogBuffer_[i][0] = '\0';
    }
}

void App::setup()
{
    Serial.begin(115200);
    delay(100);
    Wire.begin();

    hardware_.setAmplifier(false);
    hardware_.getMainDisplay().begin();
    hardware_.getMainDisplay().enableUTF8Print();
    hardware_.getSmallDisplay().begin();
    hardware_.getSmallDisplay().enableUTF8Print();

    logToSmallDisplay("Kiva Boot Agent v1.0", nullptr);

    struct BootTask {
        const char* name;
        std::function<void()> action;
    };

    std::vector<BootTask> bootTasks = {
        {"SD Card",         [&](){ if (!SdCardManager::setup()) logToSmallDisplay("SD Card", "FAIL"); }},
        {"Hardware Manager",[&](){ hardware_.setup(); }},
        {"Config Manager",  [&](){ configManager_.setup(this); }},
        {"Logger",          [&](){ Logger::getInstance().setup(); }},
        {"WiFi System",     [&](){ wifiManager_.setup(this); }},
        {"OTA System",      [&](){ otaManager_.setup(this, &wifiManager_); }},
        {"Jammer",          [&](){ jammer_.setup(this); }},
        {"Beacon Spammer",  [&](){ beaconSpammer_.setup(this); }},
        {"Deauther",        [&](){ deauther_.setup(this); }},
        {"Evil Twin",       [&](){ evilTwin_.setup(this); }},
        {"Probe Sniffer",   [&](){ probeSniffer_.setup(this); }},
        {"Karma Attacker",  [&](){ karmaAttacker_.setup(this); }},
        {"Handshake Sniffer", [&](){ handshakeCapture_.setup(this); }},
        {"BLE Manager",     [&](){ bleManager_.setup(this); }},
        {"BLE Spammer",     [&](){ bleSpammer_.setup(this); }},
        {"BadUSB",          [&](){ duckyRunner_.setup(this); }},
        {"Music Player",    [&](){ musicPlayer_.setup(this); }},
        {"Music Library",   [&](){ musicLibraryManager_.setup(this); }}
    };

    int totalTasks = bootTasks.size();
    U8G2 &mainDisplay = hardware_.getMainDisplay();
    int progressBarDrawableWidth = mainDisplay.getDisplayWidth() - 40 - 2;

    for (int i = 0; i < totalTasks; ++i) {
        logToSmallDisplay(bootTasks[i].name, "INIT");
        bootTasks[i].action();
        
        float targetFillPx = (float)(i + 1) / totalTasks * progressBarDrawableWidth;
        while (currentProgressBarFillPx_ < targetFillPx - 0.5f) {
            float diff = targetFillPx - currentProgressBarFillPx_;
            currentProgressBarFillPx_ += diff * 0.2f;
            if (currentProgressBarFillPx_ > targetFillPx) currentProgressBarFillPx_ = targetFillPx;
            updateAndDrawBootScreen(0, 0);
            delay(10); 
        }
        currentProgressBarFillPx_ = targetFillPx;
        logToSmallDisplay(bootTasks[i].name, "OK");
        delay(30);
    }
    
    currentProgressBarFillPx_ = progressBarDrawableWidth;
    updateAndDrawBootScreen(0, 0);
    logToSmallDisplay("KivaOS Loading...");
    delay(500);

    LOG(LogLevel::INFO, "App", "Boot sequence finished. Initializing menus.");
    
    menuRegistry_[MenuType::MAIN] = &mainMenu_;
    menuRegistry_[MenuType::TOOLS_CAROUSEL] = &toolsMenu_;
    menuRegistry_[MenuType::GAMES_CAROUSEL] = &gamesMenu_;
    // menuRegistry_[MenuType::SETTINGS_CAROUSEL] = &settingsMenu_;
    menuRegistry_[MenuType::SETTINGS] = &settingsMenu_;
    menuRegistry_[MenuType::BRIGHTNESS_MENU] = &brightnessMenu_;
    menuRegistry_[MenuType::UTILITIES_CAROUSEL] = &utilitiesMenu_;
    menuRegistry_[MenuType::USB_DRIVE_MODE] = &usbDriveMenu_;

    menuRegistry_[MenuType::WIFI_TOOLS_GRID] = &wifiToolsMenu_;
    menuRegistry_[MenuType::BLE_TOOLS_GRID] = &bleToolsMenu_;
    menuRegistry_[MenuType::HOST_TOOLS_GRID] = &hostToolsMenu_;
    menuRegistry_[MenuType::FIRMWARE_UPDATE_GRID] = &firmwareUpdateGrid_;
    menuRegistry_[MenuType::JAMMING_TOOLS_GRID] = &jammingToolsMenu_;
    menuRegistry_[MenuType::DEAUTH_TOOLS_GRID] = &deauthToolsMenu_;
    menuRegistry_[MenuType::BEACON_MODE_SELECTION] = &beaconModeMenu_;

    menuRegistry_[MenuType::TEXT_INPUT] = &textInputMenu_;
    menuRegistry_[MenuType::POPUP] = &popUpMenu_;

    menuRegistry_[MenuType::OTA_STATUS] = &otaStatusMenu_;
    menuRegistry_[MenuType::WIFI_CONNECTION_STATUS] = &connectionStatusMenu_;

    menuRegistry_[MenuType::WIFI_LIST] = &wifiListMenu_;
    menuRegistry_[MenuType::FIRMWARE_LIST_SD] = &firmwareListMenu_;
    menuRegistry_[MenuType::BEACON_FILE_LIST] = &beaconFileListMenu_;
    menuRegistry_[MenuType::EVIL_TWIN_PORTAL_SELECTION] = &portalListMenu_;
    menuRegistry_[MenuType::DUCKY_SCRIPT_LIST] = &duckyScriptListMenu_;

    menuRegistry_[MenuType::CHANNEL_SELECTION] = &channelSelectionMenu_;

    menuRegistry_[MenuType::JAMMING_ACTIVE] = &jammingActiveMenu_;
    menuRegistry_[MenuType::BEACON_SPAM_ACTIVE] = &beaconSpamActiveMenu_;
    menuRegistry_[MenuType::DEAUTH_ACTIVE] = &deauthActiveMenu_;
    menuRegistry_[MenuType::EVIL_TWIN_ACTIVE] = &evilTwinActiveMenu_;
    menuRegistry_[MenuType::PROBE_ACTIVE] = &probeSnifferActiveMenu_;
    menuRegistry_[MenuType::KARMA_ACTIVE] = &karmaActiveMenu_;
    menuRegistry_[MenuType::HANDSHAKE_CAPTURE_MENU] = &handshakeCaptureMenu_;
    menuRegistry_[MenuType::HANDSHAKE_CAPTURE_ACTIVE] = &handshakeCaptureActiveMenu_;
    menuRegistry_[MenuType::BLE_SPAM_ACTIVE] = &bleSpamActiveMenu_;
    menuRegistry_[MenuType::DUCKY_SCRIPT_ACTIVE] = &duckyScriptActiveMenu_;
    // --- NEW MUSIC MENUS ---
    menuRegistry_[MenuType::MUSIC_PLAYER_LIST] = &musicPlayerListMenu_;
    menuRegistry_[MenuType::NOW_PLAYING] = &nowPlayingMenu_;

    navigationStack_.clear();
    changeMenu(MenuType::MAIN, true);
    Serial.println("[APP-LOG] App setup finished.");
    LOG(LogLevel::INFO, "App", "App setup finished.");
}

void App::loop()
{
    hardware_.update();
    wifiManager_.update();
    otaManager_.loop();
    jammer_.loop();
    beaconSpammer_.loop();
    deauther_.loop();
    evilTwin_.loop();
    probeSniffer_.loop();
    karmaAttacker_.loop();
    handshakeCapture_.loop();
    bleSpammer_.loop();
    duckyRunner_.loop();

    bool wifiIsRequired = false; 
    if (currentMenu_)
    {
        MenuType currentType = currentMenu_->getMenuType();
        if (currentType == MenuType::WIFI_LIST ||
            currentType == MenuType::TEXT_INPUT ||
            currentType == MenuType::WIFI_CONNECTION_STATUS)
        {
            wifiIsRequired = true;
        }
        else if (currentType == MenuType::OTA_STATUS && otaManager_.getState() != OtaState::IDLE)
        {
            wifiIsRequired = true;
        }
    }
    
    if (wifiManager_.getState() == WifiState::CONNECTED || 
        beaconSpammer_.isActive() ||
        deauther_.isActive() || 
        evilTwin_.isActive() || 
        karmaAttacker_.isAttacking() || 
        karmaAttacker_.isSniffing())
    {
        wifiIsRequired = true;
    }

    bool hostIsActive = duckyRunner_.isActive();

    if (!wifiIsRequired && !hostIsActive && wifiManager_.isHardwareEnabled())
    {
        LOG(LogLevel::INFO, "App", "WiFi no longer required by any process. Disabling.");
        wifiManager_.setHardwareState(false);
    }

    InputEvent event = hardware_.getNextInputEvent();
    if (event != InputEvent::NONE) {
        LOG(LogLevel::DEBUG, "Input", false, "Event: %s", DebugUtils::inputEventToString(event));
        // Serial.printf("Event: %s\n", DebugUtils::inputEventToString(event));
        if (currentMenu_ != nullptr) {
            currentMenu_->handleInput(this, event);
        }
    }

    if (currentMenu_)
    {
        currentMenu_->onUpdate(this);
    }

    bool perfMode = jammer_.isActive() || beaconSpammer_.isActive() || deauther_.isActive() || 
                    evilTwin_.isActive() || karmaAttacker_.isAttacking() || bleSpammer_.isActive() ||
                    duckyRunner_.isActive();
    static unsigned long lastRenderTime = 0;
    if (perfMode && (millis() - lastRenderTime < 1000))
    {
        esp_task_wdt_reset();
        return;
    }
    lastRenderTime = millis();

    U8G2 &mainDisplay = hardware_.getMainDisplay();
    mainDisplay.clearBuffer();

    IMenu *menuForUI = currentMenu_;
    IMenu *underlyingMenu = nullptr;

    if (currentMenu_ && currentMenu_->getMenuType() == MenuType::POPUP) {
        underlyingMenu = getMenu(getPreviousMenuType());
        menuForUI = underlyingMenu; 
    }
    
    if (menuForUI) {
        if (!menuForUI->drawCustomStatusBar(this, mainDisplay)) {
            drawStatusBar();
        }
    } else if (currentMenu_) {
        drawStatusBar();
    }
    
    if (underlyingMenu) {
        underlyingMenu->draw(this, mainDisplay);
    }
    
    if (currentMenu_) {
        currentMenu_->draw(this, mainDisplay);
    }

    mainDisplay.sendBuffer();
    drawSecondaryDisplay();
}

void App::changeMenu(MenuType type, bool isForwardNav) {
    // --- LOGGING POINT #2: MENU NAVIGATION ---
    const char* fromMenuName = (currentMenu_) ? DebugUtils::menuTypeToString(currentMenu_->getMenuType()) : "NULL";
    const char* toMenuName = DebugUtils::menuTypeToString(type);
    const char* navDirection = (isForwardNav) ? "Forward" : "Back";

    // Serial.printf("[NAV] Change: From '%s' -> To '%s' (%s)\n", fromMenuName, toMenuName, navDirection);
    LOG(LogLevel::INFO, "NAV", "Change: From '%s' -> To '%s' (%s)", fromMenuName, toMenuName, navDirection);

    if (type == MenuType::NONE) return;

    MenuType exitingMenuType = MenuType::NONE;
    if (currentMenu_) {
        exitingMenuType = currentMenu_->getMenuType();
    }

    if (type == MenuType::BACK)
    {
        if (navigationStack_.size() > 1)
        {
            navigationStack_.pop_back();
            // This is not recursive. Set the new type and isForwardNav flag for this call.
            type = navigationStack_.back();
            isForwardNav = false;
        }
        else
        {
            return; // Can't go back further
        }
    }

    // Check if the DESTINATION menu requires Wi-Fi.
    bool destinationRequiresWifi = (type == MenuType::WIFI_LIST ||
                                    type == MenuType::TEXT_INPUT ||
                                    type == MenuType::WIFI_CONNECTION_STATUS);

    // If the destination requires Wi-Fi and it's currently off, turn it on NOW.
    if (destinationRequiresWifi && !wifiManager_.isHardwareEnabled())
    {
        // Serial.printf("[APP-LOGIC] Destination menu (%d) requires WiFi. Enabling hardware...\n", (int)type);
        LOG(LogLevel::DEBUG, "App", "Destination menu (%s) requires WiFi. Enabling hardware...", toMenuName);
        
        wifiManager_.setHardwareState(true);
        delay(100); // Give hardware a moment to initialize before menu uses it.
    }

    auto it = menuRegistry_.find(type);
    if (it != menuRegistry_.end())
    {
        IMenu *newMenu = it->second;
        if (currentMenu_ == newMenu && !isForwardNav)
        {
            return;
        }

        if (currentMenu_)
        {
            currentMenu_->onExit(this);
        }
        currentMenu_ = newMenu;

        if (isForwardNav || exitingMenuType != MenuType::POPUP)
        {
            currentMenu_->onEnter(this, isForwardNav); // <-- MODIFIED: Pass the flag
        }

        if (isForwardNav)
        {
            if (exitingMenuType == MenuType::POPUP)
            {
                if (!navigationStack_.empty() && navigationStack_.back() == MenuType::POPUP)
                {
                    navigationStack_.pop_back();
                }
            }

            if (navigationStack_.empty() || navigationStack_.back() != type)
            {
                navigationStack_.push_back(type);
            }
        }
    }
}

void App::replaceMenu(MenuType type) {
    LOG(LogLevel::INFO, "NAV", "Replace: From '%s' -> To '%s'", 
        (currentMenu_ ? DebugUtils::menuTypeToString(currentMenu_->getMenuType()) : "NULL"), 
        DebugUtils::menuTypeToString(type));

    if (currentMenu_) {
        currentMenu_->onExit(this);
    }
    
    // Pop the current menu from the stack
    if (!navigationStack_.empty()) {
        navigationStack_.pop_back();
    }
    
    // Find and set the new menu
    auto it = menuRegistry_.find(type);
    if (it != menuRegistry_.end()) {
        currentMenu_ = it->second;
        // Push the new menu onto the stack
        navigationStack_.push_back(type);
        // Enter the new menu as if it were a forward navigation
        currentMenu_->onEnter(this, true); 
    }
}

void App::returnToMenu(MenuType type)
{
    while (!navigationStack_.empty() && navigationStack_.back() != type)
    {
        navigationStack_.pop_back();
    }

    if (navigationStack_.empty())
    {
        changeMenu(MenuType::MAIN, true);
        return;
    }

    changeMenu(navigationStack_.back(), false);
}

IMenu *App::getMenu(MenuType type)
{
    auto it = menuRegistry_.find(type);
    return (it != menuRegistry_.end()) ? it->second : nullptr;
}

void App::showPopUp(std::string title, std::string message, PopUpMenu::OnConfirmCallback onConfirm,
                    const std::string &confirmText, const std::string &cancelText, bool executeOnConfirmBeforeExit)
{
    popUpMenu_.configure(title, message, onConfirm, confirmText, cancelText, executeOnConfirmBeforeExit);
    changeMenu(MenuType::POPUP);
}

MenuType App::getPreviousMenuType() const
{
    if (navigationStack_.size() < 2)
    {
        return MenuType::MAIN;
    }
    // The previous menu is the one before the last element (which is the pop-up itself)
    return navigationStack_[navigationStack_.size() - 2];
}

void App::drawSecondaryDisplay()
{
    if (currentMenu_ && currentMenu_->getMenuType() == MenuType::TEXT_INPUT)
    {
        // The PasswordInputMenu's draw function will handle drawing the keyboard
        // on the small display. We get the display and pass it.
        U8G2 &smallDisplay = hardware_.getSmallDisplay();
        // The cast is safe because we've checked the menu type.
        static_cast<TextInputMenu *>(currentMenu_)->draw(this, smallDisplay);
    }
    else
    {
        U8G2 &display = hardware_.getSmallDisplay();
        display.clearBuffer();
        display.setDrawColor(1);

        // Battery Info
        char batInfoStr[16];
        snprintf(batInfoStr, sizeof(batInfoStr), "%.2fV %s%d%%",
                 hardware_.getBatteryVoltage(),
                 hardware_.isCharging() ? "+" : "",
                 hardware_.getBatteryPercentage());
        display.setFont(u8g2_font_5x7_tf);
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(batInfoStr)) / 2, 8, batInfoStr);

        // Current Menu Info
        const char *modeText = "KivaOS";
        if (currentMenu_)
        {
            modeText = currentMenu_->getTitle();
        }
        display.setFont(u8g2_font_6x12_tr);
        char truncated[22];
        strncpy(truncated, modeText, sizeof(truncated) - 1);
        truncated[sizeof(truncated) - 1] = '\0';
        if (display.getStrWidth(truncated) > display.getDisplayWidth() - 4)
        {
            // A simple truncation if needed, marquee is too complex for here.
            truncated[sizeof(truncated) - 2] = '.';
            truncated[sizeof(truncated) - 3] = '.';
        }
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(truncated)) / 2, 28, truncated);

        display.sendBuffer();
    }
}

void App::drawStatusBar()
{
    U8G2 &display = hardware_.getMainDisplay();
    display.setFont(u8g2_font_6x10_tf);
    display.setDrawColor(1);

    const char *title = "KivaOS";
    if (currentMenu_ != nullptr)
    {
        title = currentMenu_->getTitle();
    }

    // Truncate title if too long
    char titleBuffer[16];
    strncpy(titleBuffer, title, sizeof(titleBuffer) - 1);
    titleBuffer[sizeof(titleBuffer) - 1] = '\0';
    if (display.getStrWidth(titleBuffer) > 65)
    {
        char *truncated = truncateText(titleBuffer, 60, display);
        strncpy(titleBuffer, truncated, sizeof(titleBuffer) - 1);
        titleBuffer[sizeof(titleBuffer) - 1] = '\0';
    }
    display.drawStr(2, 8, titleBuffer);

    // --- LOGGING POINT #4: Value Just Before Rendering ---
    float voltageToRender = hardware_.getBatteryVoltage();
    uint8_t percentToRender = hardware_.getBatteryPercentage();
    // Serial.printf("[APP-LOG-RENDER] Drawing Status Bar. Voltage: %.2fV, Percent: %d%%\n", voltageToRender, percentToRender);

    // --- Battery Drawing Logic ---
    int battery_area_x = 128 - 2;
    uint8_t batPercent = percentToRender; // Use the value we just logged
    int bat_icon_width = 10;
    battery_area_x -= bat_icon_width;
    drawBatIcon(display, battery_area_x, 2, batPercent);

    // Charging Icon
    if (hardware_.isCharging())
    {
        int charge_icon_width = 7;
        int charge_icon_spacing = 2;
        battery_area_x -= (charge_icon_width + charge_icon_spacing);
        drawCustomIcon(display, battery_area_x, 1, IconType::UI_CHARGING_BOLT, IconRenderSize::SMALL);
    }

    // Percentage Text
    char percentStr[5];
    snprintf(percentStr, sizeof(percentStr), "%d%%", batPercent);
    int percentWidth = display.getStrWidth(percentStr);
    int percent_text_spacing = 2;
    battery_area_x -= (percentWidth + percent_text_spacing);
    display.drawStr(battery_area_x, 8, percentStr);

    // Bottom line
    display.drawLine(0, STATUS_BAR_H - 1, 127, STATUS_BAR_H - 1);
}

// This function is now responsible for drawing the boot screen on the MAIN display.
// It no longer calculates progress; it just draws based on the current value
// of the member variable 'currentProgressBarFillPx_'.
void App::updateAndDrawBootScreen(unsigned long bootStartTime, unsigned long totalBootDuration)
{
    U8G2 &display = hardware_.getMainDisplay(); // Explicitly get the main display

    display.clearBuffer();
    drawCustomIcon(display, 0, -4, IconType::BOOT_LOGO);

    int progressBarY = IconSize::BOOT_LOGO_HEIGHT - 12;
    int progressBarHeight = 7;
    int progressBarWidth = display.getDisplayWidth() - 40;
    int progressBarX = (display.getDisplayWidth() - progressBarWidth) / 2;
    int progressBarDrawableWidth = progressBarWidth - 2;

    display.setDrawColor(1);
    display.drawRFrame(progressBarX, progressBarY, progressBarWidth, progressBarHeight, 1);

    int fillWidthToDraw = (int)round(currentProgressBarFillPx_);
    fillWidthToDraw = std::max(0, std::min(fillWidthToDraw, progressBarDrawableWidth));

    if (fillWidthToDraw > 0)
    {
        display.drawRBox(progressBarX + 1, progressBarY + 1, fillWidthToDraw, progressBarHeight - 2, 0);
    }

    display.sendBuffer(); // Send the buffer to the main display
}

// This function is responsible for drawing logs on the SMALL display.
void App::logToSmallDisplay(const char *message, const char *status)
{
    U8G2 &display = hardware_.getSmallDisplay(); // Explicitly get the small display
    display.setFont(u8g2_font_5x7_tf);

    char fullMessage[MAX_LOG_LINE_LENGTH_SMALL_DISPLAY];
    fullMessage[0] = '\0';
    int charsWritten = 0;

    if (status && strlen(status) > 0)
    {
        charsWritten = snprintf(fullMessage, sizeof(fullMessage), "[%s] ", status);
    }
    if (charsWritten < (int)sizeof(fullMessage) - 1)
    {
        snprintf(fullMessage + charsWritten, sizeof(fullMessage) - charsWritten, "%.*s",
                 (int)(sizeof(fullMessage) - charsWritten - 1), message);
    }

    // Scroll the log buffer up
    for (int i = 0; i < MAX_LOG_LINES_SMALL_DISPLAY - 1; ++i)
    {
        strncpy(smallDisplayLogBuffer_[i], smallDisplayLogBuffer_[i + 1], MAX_LOG_LINE_LENGTH_SMALL_DISPLAY);
    }

    // Add the new message at the bottom
    strncpy(smallDisplayLogBuffer_[MAX_LOG_LINES_SMALL_DISPLAY - 1], fullMessage, MAX_LOG_LINE_LENGTH_SMALL_DISPLAY);
    smallDisplayLogBuffer_[MAX_LOG_LINES_SMALL_DISPLAY - 1][MAX_LOG_LINE_LENGTH_SMALL_DISPLAY - 1] = '\0';

    display.clearBuffer();
    display.setDrawColor(1);
    const int lineHeight = 8;
    int yPos = display.getAscent();
    for (int i = 0; i < MAX_LOG_LINES_SMALL_DISPLAY; ++i)
    {
        display.drawStr(2, yPos + (i * lineHeight), smallDisplayLogBuffer_[i]);
    }
    display.sendBuffer(); // Send the buffer to the small display
}