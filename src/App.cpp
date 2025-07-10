#include "App.h"
#include "Icons.h" 
#include "UI_Utils.h"
#include <algorithm>
#include <cmath>
#include <SdCardManager.h>
#include <esp_task_wdt.h>

App::App() :
    currentMenu_(nullptr),
    currentProgressBarFillPx_(0.0f),
    mainMenu_(), // Uses default constructor
    toolsMenu_("Tools", {
        {"WiFi Tools", IconType::NET_WIFI, MenuType::WIFI_TOOLS_GRID},
        {"Jamming", IconType::TOOL_JAMMING, MenuType::JAMMING_TOOLS_GRID},
        {"Back", IconType::NAV_BACK, MenuType::BACK}
    }),
    gamesMenu_("Games", {
        {"Snake", IconType::GAME_SNAKE, MenuType::NONE},
        {"Tetris", IconType::GAME_TETRIS, MenuType::NONE},
        {"Back", IconType::NAV_BACK, MenuType::BACK}
    }),
    settingsMenu_("Settings", {
        {"WiFi", IconType::NET_WIFI, MenuType::WIFI_LIST},
        {"Firmware", IconType::SETTING_SYSTEM, MenuType::FIRMWARE_UPDATE_GRID},
        {"Display", IconType::SETTING_DISPLAY, MenuType::NONE},
        {"Back", IconType::NAV_BACK, MenuType::BACK}
    }),
    utilitiesMenu_("Utilities", {
        {"Vibration", IconType::UI_VIBRATION, MenuType::NONE},
        {"Laser", IconType::UI_LASER, MenuType::NONE},
        {"Back", IconType::NAV_BACK, MenuType::BACK}
    }),
    wifiToolsMenu_("WiFi Tools", {
        {"Beacon Spam", IconType::TOOL_INJECTION, MenuType::NONE},
        {"Deauth", IconType::TOOL_JAMMING, MenuType::NONE},
        {"Back", IconType::NAV_BACK, MenuType::BACK}
    }, 2),
    firmwareUpdateGrid_("Update", {
        {"Web Update", IconType::NET_WIFI, MenuType::NONE}, // Logic handled in GridMenu
        {"SD Card", IconType::INFO, MenuType::FIRMWARE_LIST_SD}, // Can navigate directly
        {"Basic OTA", IconType::TOOL_INJECTION, MenuType::NONE}, // Logic handled in GridMenu
        {"Back", IconType::NAV_BACK, MenuType::BACK}
    }, 2),
    wifiListMenu_(),
    textInputMenu_(),
    connectionStatusMenu_(),
    popUpMenu_(),
    firmwareListMenu_(),
    otaStatusMenu_()
{
    // Initialize the small display log buffer
    for (int i = 0; i < MAX_LOG_LINES_SMALL_DISPLAY; ++i) {
        smallDisplayLogBuffer_[i][0] = '\0';
    }
}

void App::setup() {
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
    while ((currentTime = millis()) - bootStartTime < TOTAL_BOOT_DURATION_MS) {
        unsigned long elapsedTime = currentTime - bootStartTime;

        // Perform boot tasks as we pass their milestones
        if (!i2c_done && elapsedTime >= MILESTONE_I2C) {
            logToSmallDisplay("I2C Bus Initialized", "OK");
            i2c_done = true;
        }
        if (!hw_done && elapsedTime >= MILESTONE_HW_MANAGER) {
            hardware_.setup();
            logToSmallDisplay("Hardware Manager", "OK");
            hw_done = true;
        }
        if (!wifi_done && elapsedTime >= MILESTONE_WIFI) {
            logToSmallDisplay("WiFi Subsystem", "INIT");
            wifi_done = true;
        }
        if (!jammer_done && elapsedTime >= MILESTONE_JAMMER) {
            logToSmallDisplay("Jamming System", "INIT");
            jammer_done = true;
        }
        if (!ready_done && elapsedTime >= MILESTONE_READY) {
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
    delay(500); // Pause on the full bar for a moment

    // --- Boot Sequence End. Transition to main application ---
    Serial.println("[APP-LOG] Boot sequence finished. Initializing menus.");

    SdCardManager::setup(); // Setup SD Card first
    wifiManager_.setup(this);   // Then WifiManager which may depend on it. Pass App context.
    otaManager_.setup(this, &wifiManager_);
    
    // Register all menus
    menuRegistry_[MenuType::MAIN] = &mainMenu_;
    menuRegistry_[MenuType::TOOLS_CAROUSEL] = &toolsMenu_;
    menuRegistry_[MenuType::GAMES_CAROUSEL] = &gamesMenu_;
    menuRegistry_[MenuType::SETTINGS_CAROUSEL] = &settingsMenu_;
    menuRegistry_[MenuType::UTILITIES_CAROUSEL] = &utilitiesMenu_;
    menuRegistry_[MenuType::WIFI_TOOLS_GRID] = &wifiToolsMenu_;
    menuRegistry_[MenuType::WIFI_LIST] = &wifiListMenu_;
    menuRegistry_[MenuType::TEXT_INPUT] = &textInputMenu_;
    menuRegistry_[MenuType::WIFI_CONNECTION_STATUS] = &connectionStatusMenu_;
    menuRegistry_[MenuType::POPUP] = &popUpMenu_;
    menuRegistry_[MenuType::FIRMWARE_UPDATE_GRID] = &firmwareUpdateGrid_;
    menuRegistry_[MenuType::FIRMWARE_LIST_SD] = &firmwareListMenu_;
    menuRegistry_[MenuType::OTA_STATUS] = &otaStatusMenu_;
    
    navigationStack_.clear();
    changeMenu(MenuType::MAIN, true);
    Serial.println("[APP-LOG] App setup finished.");
}

void App::loop() {
    hardware_.update();
    wifiManager_.update();
    otaManager_.loop();

    // --- REFINED WiFi Power Management Logic ---
    bool wifiIsRequired = false;
    if (currentMenu_) {
        MenuType currentType = currentMenu_->getMenuType();
        // Menus that ALWAYS require WiFi to be on
        if (currentType == MenuType::WIFI_LIST ||
            currentType == MenuType::TEXT_INPUT ||
            currentType == MenuType::WIFI_CONNECTION_STATUS)
        {
            wifiIsRequired = true;
        }
        // The OTA Status menu ONLY requires WiFi if an OTA process is actually running
        else if (currentType == MenuType::OTA_STATUS && otaManager_.getState() != OtaState::IDLE) {
            wifiIsRequired = true;
        }
    }
    // Also keep WiFi on if we are connected for other reasons (e.g. background task)
    if (wifiManager_.getState() == WifiState::CONNECTED) {
        wifiIsRequired = true;
    }

    // Auto-disable WiFi if it's enabled but no longer required by the current context
    if (!wifiIsRequired && wifiManager_.isHardwareEnabled()) {
        wifiManager_.setHardwareState(false);
    }

    InputEvent event = hardware_.getNextInputEvent();
    if (event != InputEvent::NONE && currentMenu_ != nullptr) {
        currentMenu_->handleInput(this, event);
    }

    if (currentMenu_) {
        currentMenu_->onUpdate(this);
    }

    U8G2& mainDisplay = hardware_.getMainDisplay();
    mainDisplay.clearBuffer();
    
    if (currentMenu_ && currentMenu_->getMenuType() == MenuType::POPUP) {
        IMenu* underlyingMenu = getMenu(getPreviousMenuType());
        if (underlyingMenu) {
            drawStatusBar();
            underlyingMenu->draw(this, mainDisplay);
        }
    } else if (currentMenu_) {
        drawStatusBar();
    }
    
    if (currentMenu_) {
        currentMenu_->draw(this, mainDisplay);
    }

    mainDisplay.sendBuffer();
    drawSecondaryDisplay();
}

void App::changeMenu(MenuType type, bool isForwardNav) {
    if (type == MenuType::NONE) return;

    MenuType exitingMenuType = MenuType::NONE;
    if (currentMenu_) {
        exitingMenuType = currentMenu_->getMenuType();
    }

    if (type == MenuType::BACK) {
        if (navigationStack_.size() > 1) {
            navigationStack_.pop_back();
            // This is not recursive. Set the new type and isForwardNav flag for this call.
            type = navigationStack_.back(); 
            isForwardNav = false;
        } else {
            return; // Can't go back further
        }
    }

    // Check if the DESTINATION menu requires Wi-Fi.
    bool destinationRequiresWifi = (type == MenuType::WIFI_LIST ||
                                    type == MenuType::TEXT_INPUT ||
                                    type == MenuType::WIFI_CONNECTION_STATUS);

    // If the destination requires Wi-Fi and it's currently off, turn it on NOW.
    if (destinationRequiresWifi && !wifiManager_.isHardwareEnabled()) {
        Serial.printf("[APP-LOGIC] Destination menu (%d) requires WiFi. Enabling hardware...\n", (int)type);
        wifiManager_.setHardwareState(true);
        delay(100); // Give hardware a moment to initialize before menu uses it.
    }

    auto it = menuRegistry_.find(type);
    if (it != menuRegistry_.end()) {
        IMenu* newMenu = it->second;
        if (currentMenu_ == newMenu && !isForwardNav) {
            return;
        }

        if (currentMenu_) {
            currentMenu_->onExit(this);
        }
        currentMenu_ = newMenu;

        if (isForwardNav || exitingMenuType != MenuType::POPUP) {
            currentMenu_->onEnter(this);
        }

        if (isForwardNav) {
            if (navigationStack_.empty() || navigationStack_.back() != type) {
                 navigationStack_.push_back(type);
            }
        }
    }
}

void App::returnToMenu(MenuType type) {
    while (!navigationStack_.empty() && navigationStack_.back() != type) {
        navigationStack_.pop_back();
    }
    
    if (navigationStack_.empty()) {
        changeMenu(MenuType::MAIN, true);
        return;
    }

    changeMenu(navigationStack_.back(), false);
}

IMenu* App::getMenu(MenuType type) {
    auto it = menuRegistry_.find(type);
    return (it != menuRegistry_.end()) ? it->second : nullptr;
}

void App::showPopUp(std::string title, std::string message, PopUpMenu::OnConfirmCallback onConfirm) {
    popUpMenu_.configure(title, message, onConfirm);
    changeMenu(MenuType::POPUP);
}

// --- NEW/MODIFIED METHODS ---

void App::setPostWifiAction(std::function<void(App*)> action) {
    postWifiAction_ = action;
}

void App::executePostWifiAction() {
    if (postWifiAction_) {
        Serial.println("[APP-LOG] Executing post-wifi connection action.");
        postWifiAction_(this);
        postWifiAction_ = nullptr; // Clear it after execution
    }
}

MenuType App::getPreviousMenuType() const {
    if (navigationStack_.size() < 2) {
        return MenuType::MAIN;
    }
    // The previous menu is the one before the last element (which is the pop-up itself)
    return navigationStack_[navigationStack_.size() - 2];
}

void App::drawSecondaryDisplay() {
    if (currentMenu_ && currentMenu_->getMenuType() == MenuType::TEXT_INPUT) {
        // The PasswordInputMenu's draw function will handle drawing the keyboard
        // on the small display. We get the display and pass it.
        U8G2& smallDisplay = hardware_.getSmallDisplay();
        // The cast is safe because we've checked the menu type.
        static_cast<TextInputMenu*>(currentMenu_)->draw(this, smallDisplay);
    } else {
        U8G2& display = hardware_.getSmallDisplay();
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
        const char* modeText = "KivaOS";
        if (currentMenu_) {
            modeText = currentMenu_->getTitle();
        }
        display.setFont(u8g2_font_6x12_tr);
        char truncated[22];
        strncpy(truncated, modeText, sizeof(truncated) - 1);
        truncated[sizeof(truncated) - 1] = '\0';
        if(display.getStrWidth(truncated) > display.getDisplayWidth() - 4) {
            // A simple truncation if needed, marquee is too complex for here.
            truncated[sizeof(truncated) - 2] = '.';
            truncated[sizeof(truncated) - 3] = '.';
        }
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(truncated)) / 2, 28, truncated);
        
        display.sendBuffer();
    }
}

void App::drawStatusBar() {
    U8G2& display = hardware_.getMainDisplay();
    display.setFont(u8g2_font_6x10_tf);
    display.setDrawColor(1);
    
    const char* title = "KivaOS"; 
    if (currentMenu_ != nullptr) {
        title = currentMenu_->getTitle();
    }
    
    // Truncate title if too long
    char titleBuffer[16];
    strncpy(titleBuffer, title, sizeof(titleBuffer) - 1);
    titleBuffer[sizeof(titleBuffer) - 1] = '\0';
    if (display.getStrWidth(titleBuffer) > 65) {
        char* truncated = truncateText(titleBuffer, 60, display);
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
    if (hardware_.isCharging()) {
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

void App::updateAndDrawBootScreen(unsigned long bootStartTime, unsigned long totalBootDuration) {
    U8G2& display = hardware_.getMainDisplay();

    float progress_t = (float)(millis() - bootStartTime) / totalBootDuration;
    progress_t = std::max(0.0f, std::min(1.0f, progress_t));

    float eased_t = progress_t < 0.5f 
                    ? 4.0f * progress_t * progress_t * progress_t 
                    : 1.0f - pow(-2.0f * progress_t + 2.0f, 3) / 2.0f;

    int progressBarDrawableWidth = display.getDisplayWidth() - 40 - 2;
    currentProgressBarFillPx_ = progressBarDrawableWidth * eased_t;

    display.clearBuffer();
    drawCustomIcon(display, 0, -2, IconType::BOOT_LOGO);

    int progressBarY = IconSize::BOOT_LOGO_HEIGHT - 12;
    int progressBarHeight = 7;
    int progressBarWidth = display.getDisplayWidth() - 40;
    int progressBarX = (display.getDisplayWidth() - progressBarWidth) / 2;

    display.setDrawColor(1);
    display.drawRFrame(progressBarX, progressBarY, progressBarWidth, progressBarHeight, 1);

    int fillWidthToDraw = (int)round(currentProgressBarFillPx_);
    fillWidthToDraw = std::max(0, std::min(fillWidthToDraw, progressBarDrawableWidth));

    if (fillWidthToDraw > 0) {
        display.drawRBox(progressBarX + 1, progressBarY + 1, fillWidthToDraw, progressBarHeight - 2, 0);
    }
    
    display.sendBuffer();
}

void App::logToSmallDisplay(const char* message, const char* status) {
    U8G2& display = hardware_.getSmallDisplay();
    display.setFont(u8g2_font_5x7_tf);

    char fullMessage[MAX_LOG_LINE_LENGTH_SMALL_DISPLAY];
    fullMessage[0] = '\0';
    int charsWritten = 0;

    if (status && strlen(status) > 0) {
        charsWritten = snprintf(fullMessage, sizeof(fullMessage), "[%s] ", status);
    }
    if (charsWritten < (int)sizeof(fullMessage) - 1) {
        snprintf(fullMessage + charsWritten, sizeof(fullMessage) - charsWritten, "%.*s",
                 (int)(sizeof(fullMessage) - charsWritten - 1), message);
    }

    for (int i = 0; i < MAX_LOG_LINES_SMALL_DISPLAY - 1; ++i) {
        strncpy(smallDisplayLogBuffer_[i], smallDisplayLogBuffer_[i + 1], MAX_LOG_LINE_LENGTH_SMALL_DISPLAY);
    }

    strncpy(smallDisplayLogBuffer_[MAX_LOG_LINES_SMALL_DISPLAY - 1], fullMessage, MAX_LOG_LINE_LENGTH_SMALL_DISPLAY);
    smallDisplayLogBuffer_[MAX_LOG_LINES_SMALL_DISPLAY - 1][MAX_LOG_LINE_LENGTH_SMALL_DISPLAY - 1] = '\0';

    display.clearBuffer();
    display.setDrawColor(1);
    const int lineHeight = 8;
    int yPos = display.getAscent();
    for (int i = 0; i < MAX_LOG_LINES_SMALL_DISPLAY; ++i) {
        display.drawStr(2, yPos + (i * lineHeight), smallDisplayLogBuffer_[i]);
    }
    display.sendBuffer();
}