#include "App.h"
#include "Icons.h" 
#include "UI_Utils.h"



App::App() :
    currentMenu_(nullptr),
    toolsMenu_("Tools", {
        {"WiFi Tools", IconType::NET_WIFI, MenuType::WIFI_TOOLS_GRID},
        {"Jamming", IconType::TOOL_JAMMING, MenuType::NONE},
        {"Back", IconType::NAV_BACK, MenuType::BACK}
    }),
    gamesMenu_("Games", {
        {"Snake", IconType::GAME_SNAKE, MenuType::NONE},
        {"Tetris", IconType::GAME_TETRIS, MenuType::NONE},
        {"Back", IconType::NAV_BACK, MenuType::BACK}
    }),
    settingsMenu_("Settings", {
        {"WiFi", IconType::NET_WIFI, MenuType::NONE},
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
    }, 2)
{
}

void App::setup() {
    Serial.begin(115200);
    // Give a moment for Serial to initialize before logging
    delay(500); 
    Serial.println("\n\n[APP-LOG] App setup started.");

    hardware_.setup();
    
    menuRegistry_[MenuType::MAIN] = &mainMenu_;
    menuRegistry_[MenuType::TOOLS_CAROUSEL] = &toolsMenu_;
    menuRegistry_[MenuType::GAMES_CAROUSEL] = &gamesMenu_;
    menuRegistry_[MenuType::SETTINGS_CAROUSEL] = &settingsMenu_;
    menuRegistry_[MenuType::UTILITIES_CAROUSEL] = &utilitiesMenu_;
    menuRegistry_[MenuType::WIFI_TOOLS_GRID] = &wifiToolsMenu_;
    
    navigationStack_.clear();
    changeMenu(MenuType::MAIN, true);
    Serial.println("[APP-LOG] App setup finished.");
}

void App::loop() {
    hardware_.update();
    
    InputEvent event = hardware_.getNextInputEvent();
    if (event != InputEvent::NONE) {
        Serial.printf("[APP-LOG] Received event: %d. Passing to current menu.\n", static_cast<int>(event));
        if (currentMenu_ != nullptr) {
            currentMenu_->handleInput(this, event);
        } else {
            Serial.println("[APP-LOG] WARNING: Event received but no current menu to handle it!");
        }
    }

    if (currentMenu_) {
        currentMenu_->onUpdate(this);
    }
    
    U8G2& mainDisplay = hardware_.getMainDisplay();
    mainDisplay.clearBuffer();
    drawStatusBar();
    if (currentMenu_) {
        currentMenu_->draw(this, mainDisplay);
    }
    mainDisplay.sendBuffer();

    drawSecondaryDisplay();
}

void App::drawSecondaryDisplay() {
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

void App::changeMenu(MenuType type, bool isForwardNav) {
    if (type == MenuType::NONE) return;

    // Add logging to menu changes
    Serial.printf("[APP-LOG] changeMenu called. Type: %d, isForward: %d\n", static_cast<int>(type), isForwardNav);

    if (type == MenuType::BACK) {
        if (navigationStack_.size() > 1) {
            navigationStack_.pop_back();
            // The recursive call will be logged
            changeMenu(navigationStack_.back(), false);
        } else {
             Serial.println("[APP-LOG] BACK ignored, at bottom of stack.");
        }
        return;
    }

    auto it = menuRegistry_.find(type);
    if (it != menuRegistry_.end()) {
        IMenu* newMenu = it->second;
        if (currentMenu_ == newMenu && !isForwardNav) {
            Serial.println("[APP-LOG] changeMenu ignored, already on target menu during back navigation.");
            return;
        }

        if (currentMenu_) {
            Serial.printf("[APP-LOG] Exiting menu: %s\n", currentMenu_->getTitle());
            currentMenu_->onExit(this);
        }
        currentMenu_ = newMenu;
        Serial.printf("[APP-LOG] Entering menu: %s\n", currentMenu_->getTitle());
        currentMenu_->onEnter(this);

        if (isForwardNav) {
            if (navigationStack_.empty() || navigationStack_.back() != type) {
                 navigationStack_.push_back(type);
            }
        }
    } else {
        Serial.printf("[APP-LOG] WARNING: Menu type %d not found in registry!\n", static_cast<int>(type));
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
    Serial.printf("[APP-LOG-RENDER] Drawing Status Bar. Voltage: %.2fV, Percent: %d%%\n", voltageToRender, percentToRender);

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