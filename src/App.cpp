#include "App.h"
#include "Icons.h"
#include "UI_Utils.h"
#include "Jammer.h"
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
        MenuItem{"Jamming", IconType::TOOL_JAMMING, MenuType::JAMMING_TOOLS_GRID},
        MenuItem{"Back", IconType::NAV_BACK, MenuType::BACK}}),
    gamesMenu_("Games", {
        MenuItem{"Snake", IconType::GAME_SNAKE, MenuType::NONE},
        MenuItem{"Tetris", IconType::GAME_TETRIS, MenuType::NONE},
        MenuItem{"Back", IconType::NAV_BACK, MenuType::BACK}}),
    settingsMenu_("Settings", {
        MenuItem{"WiFi", IconType::NET_WIFI, MenuType::NONE, 
            [](App *app) {
                // --- MODIFIED: Use the data source now ---
                app->getWifiListDataSource().setScanOnEnter(true);
                app->changeMenu(MenuType::WIFI_LIST);
            }},
        MenuItem{"Firmware", IconType::SETTING_SYSTEM, MenuType::FIRMWARE_UPDATE_GRID},
        MenuItem{"Display", IconType::SETTING_DISPLAY, MenuType::NONE},
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
        MenuItem{"Back", IconType::NAV_BACK, MenuType::BACK}
    }),
    wifiToolsMenu_("WiFi Tools", {
        MenuItem{"Beacon Spam", IconType::TOOL_JAMMING, MenuType::BEACON_MODE_SELECTION},
        MenuItem{"Deauth", IconType::DISCONNECT, MenuType::DEAUTH_TOOLS_GRID},
        // --- MODIFIED ATTACK FLOW ---
        MenuItem{"Evil Twin", IconType::SKULL, MenuType::EVIL_TWIN_PORTAL_SELECTION},
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
                    cfg.technique = JammingTechnique::NOISE_INJECTION; // Correct for BLE
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
                    cfg.technique = JammingTechnique::CONSTANT_CARRIER; // Correct for BT
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
                    cfg.technique = JammingTechnique::NOISE_INJECTION; // --- REVERTED FOR FAST SWEEP ---
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
                    cfg.technique = JammingTechnique::CONSTANT_CARRIER; // Correct for wide sweep
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
                        // The sample code uses noise injection (sending junk packets).
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
    wifiListDataSource_(),
    firmwareListDataSource_(),
    beaconFileListDataSource_(),
    wifiListMenu_("Wi-Fi Setup", MenuType::WIFI_LIST, &wifiListDataSource_),
    firmwareListMenu_("Update from SD", MenuType::FIRMWARE_LIST_SD, &firmwareListDataSource_),
    beaconFileListMenu_("Select SSID File", MenuType::BEACON_FILE_LIST, &beaconFileListDataSource_),
    portalListMenu_("Select Portal", MenuType::EVIL_TWIN_PORTAL_SELECTION, &portalListDataSource_),
    beaconSpamActiveMenu_(),
    deauthActiveMenu_(),
    evilTwinActiveMenu_()
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

    // --- NEW CONTINUOUS BOOT SEQUENCE ---
    Serial.println("\n[APP-LOG] Starting Kiva Boot Sequence...");

    // Initialize displays early
    hardware_.getMainDisplay().begin();
    hardware_.getMainDisplay().enableUTF8Print();
    hardware_.getSmallDisplay().begin();
    hardware_.getSmallDisplay().enableUTF8Print();

    const unsigned long TOTAL_BOOT_DURATION_MS = 3000;
    const unsigned long bootStartTime = millis();
    unsigned long currentTime = 0;

    // Define boot task milestones (in milliseconds)
    const unsigned long MILESTONE_I2C = 200;
    const unsigned long MILESTONE_HW_MANAGER = 800;
    const unsigned long MILESTONE_WIFI = 1400;
    const unsigned long MILESTONE_JAMMER = 2000;
    const unsigned long MILESTONE_READY = 2600;

    bool i2c_done = false, hw_done = false, wifi_done = false, jammer_done = false, ready_done = false;

    logToSmallDisplay("Kiva Boot Agent v1.0", nullptr);

    // This loop runs for the total duration of the boot animation.
    while ((currentTime = millis()) - bootStartTime < TOTAL_BOOT_DURATION_MS)
    {
        unsigned long elapsedTime = currentTime - bootStartTime;

        // Perform boot tasks as we pass their milestones
        if (!i2c_done && elapsedTime >= MILESTONE_I2C)
        {
            logToSmallDisplay("I2C Bus Initialized", "OK");
            i2c_done = true;
        }
        if (!hw_done && elapsedTime >= MILESTONE_HW_MANAGER)
        {
            hardware_.setup();
            logToSmallDisplay("Hardware Manager", "OK");
            hw_done = true;
        }
        if (!wifi_done && elapsedTime >= MILESTONE_WIFI)
        {
            logToSmallDisplay("WiFi Subsystem", "INIT");
            wifi_done = true;
        }
        if (!jammer_done && elapsedTime >= MILESTONE_JAMMER)
        {
            logToSmallDisplay("Jamming System", "INIT");
            jammer_done = true;
        }
        if (!ready_done && elapsedTime >= MILESTONE_READY)
        {
            logToSmallDisplay("Main Display OK", "READY");
            ready_done = true;
        }

        // Continuously update and draw the smooth progress bar
        updateAndDrawBootScreen(bootStartTime, TOTAL_BOOT_DURATION_MS);
        delay(16); // Maintain ~60 FPS update rate
    }

    // Ensure the bar is 100% full at the end and show the final log
    logToSmallDisplay("KivaOS Loading...");
    updateAndDrawBootScreen(bootStartTime, TOTAL_BOOT_DURATION_MS); // Final draw at 100%
    delay(500);                                                     // Pause on the full bar for a moment

    // --- Boot Sequence End. Transition to main application ---
    // Serial.println("[APP-LOG] Boot sequence finished. Initializing menus.");

    SdCardManager::setup();   // Setup SD Card first

    Logger::getInstance().setup();

    LOG(LogLevel::INFO, "App", "Boot sequence finished. Initializing managers.");

    wifiManager_.setup(this); // Then WifiManager which may depend on it. Pass App context.
    otaManager_.setup(this, &wifiManager_);
    jammer_.setup(this);
    beaconSpammer_.setup(this);
    deauther_.setup(this);
    evilTwin_.setup(this);

    // Register all menus
    menuRegistry_[MenuType::MAIN] = &mainMenu_;
    menuRegistry_[MenuType::TOOLS_CAROUSEL] = &toolsMenu_;
    menuRegistry_[MenuType::GAMES_CAROUSEL] = &gamesMenu_;
    menuRegistry_[MenuType::SETTINGS_CAROUSEL] = &settingsMenu_;
    menuRegistry_[MenuType::UTILITIES_CAROUSEL] = &utilitiesMenu_;

    menuRegistry_[MenuType::WIFI_TOOLS_GRID] = &wifiToolsMenu_;
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

    menuRegistry_[MenuType::CHANNEL_SELECTION] = &channelSelectionMenu_;

    menuRegistry_[MenuType::JAMMING_ACTIVE] = &jammingActiveMenu_;
    menuRegistry_[MenuType::BEACON_SPAM_ACTIVE] = &beaconSpamActiveMenu_;
    menuRegistry_[MenuType::DEAUTH_ACTIVE] = &deauthActiveMenu_;
    menuRegistry_[MenuType::EVIL_TWIN_ACTIVE] = &evilTwinActiveMenu_;

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

    // --- REFINED WiFi Power Management Logic ---
    bool wifiIsRequired = false;
    if (currentMenu_)
    {
        MenuType currentType = currentMenu_->getMenuType();
        // Menus that ALWAYS require WiFi to be on
        if (currentType == MenuType::WIFI_LIST ||
            currentType == MenuType::TEXT_INPUT ||
            currentType == MenuType::WIFI_CONNECTION_STATUS)
        {
            wifiIsRequired = true;
        }
        // The OTA Status menu ONLY requires WiFi if an OTA process is actually running
        else if (currentType == MenuType::OTA_STATUS && otaManager_.getState() != OtaState::IDLE)
        {
            wifiIsRequired = true;
        }
    }
    // Also keep WiFi on if we are connected for other reasons (e.g. background task)
    if (wifiManager_.getState() == WifiState::CONNECTED || beaconSpammer_.isActive() ||
        deauther_.isActive() || evilTwin_.isActive())
    {
        wifiIsRequired = true;
    }

    // Auto-disable WiFi if it's enabled but no longer required by the current context
    if (!wifiIsRequired && wifiManager_.isHardwareEnabled())
    {
        wifiManager_.setHardwareState(false);
    }

    InputEvent event = hardware_.getNextInputEvent();
    if (event != InputEvent::NONE) {
        // LOG(LogLevel::DEBUG, "Input", "Event: %s", DebugUtils::inputEventToString(event));
        Serial.printf("[INPUT] Event: %s\n", DebugUtils::inputEventToString(event));
        if (currentMenu_ != nullptr) {
            currentMenu_->handleInput(this, event);
        }
    }

    if (currentMenu_)
    {
        currentMenu_->onUpdate(this);
    }

    // --- PERFORMANCE MODE RENDERING THROTTLE ---
    bool perfMode = jammer_.isActive() || beaconSpammer_.isActive() || deauther_.isActive() || evilTwin_.isActive();
    static unsigned long lastRenderTime = 0;
    // Update screen only once per second in performance mode.
    // This frees up massive amounts of CPU time for the attack loops.
    if (perfMode && (millis() - lastRenderTime < 1000))
    {
        // In performance mode, we skip drawing to save CPU time,
        // but feed the watchdog timer to prevent reboots.
        esp_task_wdt_reset();
        return;
    }
    lastRenderTime = millis();

    U8G2 &mainDisplay = hardware_.getMainDisplay();
    mainDisplay.clearBuffer();

    if (currentMenu_ && currentMenu_->getMenuType() == MenuType::POPUP)
    {
        IMenu *underlyingMenu = getMenu(getPreviousMenuType());
        if (underlyingMenu)
        {
            drawStatusBar();
            underlyingMenu->draw(this, mainDisplay);
        }
    }
    else if (currentMenu_)
    {
        drawStatusBar();
    }

    if (currentMenu_)
    {
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
            currentMenu_->onEnter(this);
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

void App::updateAndDrawBootScreen(unsigned long bootStartTime, unsigned long totalBootDuration)
{
    U8G2 &display = hardware_.getMainDisplay();

    float progress_t = (float)(millis() - bootStartTime) / totalBootDuration;
    progress_t = std::max(0.0f, std::min(1.0f, progress_t));

    float eased_t = progress_t < 0.5f
                        ? 4.0f * progress_t * progress_t * progress_t
                        : 1.0f - pow(-2.0f * progress_t + 2.0f, 3) / 2.0f;

    int progressBarDrawableWidth = display.getDisplayWidth() - 40 - 2;
    currentProgressBarFillPx_ = progressBarDrawableWidth * eased_t;

    display.clearBuffer();
    drawCustomIcon(display, 0, -4, IconType::BOOT_LOGO);

    int progressBarY = IconSize::BOOT_LOGO_HEIGHT - 12;
    int progressBarHeight = 7;
    int progressBarWidth = display.getDisplayWidth() - 40;
    int progressBarX = (display.getDisplayWidth() - progressBarWidth) / 2;

    display.setDrawColor(1);
    display.drawRFrame(progressBarX, progressBarY, progressBarWidth, progressBarHeight, 1);

    int fillWidthToDraw = (int)round(currentProgressBarFillPx_);
    fillWidthToDraw = std::max(0, std::min(fillWidthToDraw, progressBarDrawableWidth));

    if (fillWidthToDraw > 0)
    {
        display.drawRBox(progressBarX + 1, progressBarY + 1, fillWidthToDraw, progressBarHeight - 2, 0);
    }

    display.sendBuffer();
}

void App::logToSmallDisplay(const char *message, const char *status)
{
    U8G2 &display = hardware_.getSmallDisplay();
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

    for (int i = 0; i < MAX_LOG_LINES_SMALL_DISPLAY - 1; ++i)
    {
        strncpy(smallDisplayLogBuffer_[i], smallDisplayLogBuffer_[i + 1], MAX_LOG_LINE_LENGTH_SMALL_DISPLAY);
    }

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
    display.sendBuffer();
}