#include "SettingsMenu.h"
#include "App.h"
#include "UI_Utils.h"
#include "ConfigManager.h"
#include "Firmware.h"

SettingsMenu::SettingsMenu() : 
    selectedIndex_(0)
{
    menuItems_ = {
        {"Brightness", MenuType::NONE, nullptr, true},
        {"Volume", MenuType::NONE, nullptr, true}, // New volume slider
        {"Hop Delay", MenuType::NONE, nullptr, true},
        {"KB Layout", MenuType::NONE, nullptr, false},
        {"OTA Password", MenuType::NONE, nullptr, false},
        {"WiFi Settings", MenuType::WIFI_LIST, nullptr, false},
        {"Firmware Update", MenuType::FIRMWARE_UPDATE_GRID, nullptr, false},
        {"System Info", MenuType::NONE, nullptr, false},
        {"Reload from SD", MenuType::NONE, [](App* app) {
            if (app->getConfigManager().reloadFromSdCard()) {
                app->showPopUp("Success", "Settings reloaded from SD card.", nullptr, "OK", "", true);
            } else {
                app->showPopUp("Error", "Failed to load settings from SD card.", nullptr, "OK", "", true);
            }
        }, false},
        {"Back", MenuType::BACK, nullptr, false}
    };
}

void SettingsMenu::onEnter(App* app, bool isForwardNav) {
    if (isForwardNav) {
        selectedIndex_ = 0;
    }
    animation_.resize(menuItems_.size());
    animation_.init();
    animation_.startIntro(selectedIndex_, menuItems_.size());
}

void SettingsMenu::onUpdate(App* app) {
    animation_.update();
}

void SettingsMenu::onExit(App* app) {}

void SettingsMenu::handleInput(App* app, InputEvent event) {
    const auto& selectedItem = menuItems_[selectedIndex_];

    if (selectedItem.label == "Brightness") { 
        if (event == InputEvent::BTN_LEFT_PRESS || event == InputEvent::ENCODER_CCW) {
            changeBrightness(app, -13, "Unified");
            return;
        }
        if (event == InputEvent::BTN_RIGHT_PRESS || event == InputEvent::ENCODER_CW) {
            changeBrightness(app, 13, "Unified");
            return;
        }
    } else if (selectedItem.label == "Volume") {
        if (event == InputEvent::BTN_LEFT_PRESS || event == InputEvent::ENCODER_CCW) {
            changeVolume(app, -5);
            return;
        }
        if (event == InputEvent::BTN_RIGHT_PRESS || event == InputEvent::ENCODER_CW) {
            changeVolume(app, 5);
            return;
        }
    } else if (selectedItem.label == "Hop Delay") {
        if (event == InputEvent::BTN_LEFT_PRESS || event == InputEvent::ENCODER_CCW) {
            changeChannelHopDelay(app, -500);
            return;
        }
        if (event == InputEvent::BTN_RIGHT_PRESS || event == InputEvent::ENCODER_CW) {
            changeChannelHopDelay(app, 500);
            return;
        }
    } else if (selectedItem.label == "KB Layout") {
        if (event == InputEvent::BTN_LEFT_PRESS || event == InputEvent::ENCODER_CCW) {
            changeKeyboardLayout(app, -1);
            return;
        }
        if (event == InputEvent::BTN_RIGHT_PRESS || event == InputEvent::ENCODER_CW) {
            changeKeyboardLayout(app, 1);
            return;
        }
    }

    switch(event) {
        case InputEvent::BTN_DOWN_PRESS: scroll(1); break;
        case InputEvent::BTN_UP_PRESS: scroll(-1); break;
        case InputEvent::BTN_A_PRESS:
        case InputEvent::BTN_OK_PRESS:
        case InputEvent::BTN_ENCODER_PRESS:
            if (selectedItem.label == "Brightness") {
                app->changeMenu(MenuType::BRIGHTNESS_MENU);
            }
            else if (selectedItem.label == "OTA Password") {
                TextInputMenu& textMenu = app->getTextInputMenu();
                textMenu.configure("New OTA Password", 
                    [](App* cb_app, const char* new_password) {
                        if (strlen(new_password) > 0 && strlen(new_password) < Firmware::MIN_AP_PASSWORD_LEN) {
                            cb_app->showPopUp("Error", "Password must be at least 8 characters.", nullptr, "OK", "", true);
                        } else {
                            auto& settings = cb_app->getConfigManager().getSettings();
                            strlcpy(settings.otaPassword, new_password, sizeof(settings.otaPassword));
                            cb_app->getConfigManager().saveSettings();
                            cb_app->replaceMenu(MenuType::SETTINGS);
                            cb_app->showPopUp("Success", "OTA Password Saved.", nullptr, "OK", "", true);
                        }
                    }, 
                    false,
                    app->getConfigManager().getSettings().otaPassword
                );
                app->changeMenu(MenuType::TEXT_INPUT);
            } 
            else if (selectedItem.action) {
                selectedItem.action(app);
            } else if (selectedItem.targetMenu != MenuType::NONE) {
                app->changeMenu(selectedItem.targetMenu);
            }
            break;
        case InputEvent::BTN_BACK_PRESS: app->changeMenu(MenuType::MAIN); break;
        default: break;
    }
}

void SettingsMenu::scroll(int direction) {
    if (menuItems_.empty()) return;
    selectedIndex_ = (selectedIndex_ + direction + menuItems_.size()) % menuItems_.size();
    animation_.setTargets(selectedIndex_, menuItems_.size());
}

void SettingsMenu::changeBrightness(App* app, int delta, const std::string& target) {
    auto& settings = app->getConfigManager().getSettings();
    
    if (target == "Unified") {
        int newVal = settings.mainDisplayBrightness + delta;
        if (newVal < 0) newVal = 0;
        if (newVal > 255) newVal = 255;
        
        settings.mainDisplayBrightness = newVal;
        settings.auxDisplayBrightness = newVal;
    } 
    
    app->getConfigManager().saveSettings();
}

void SettingsMenu::changeVolume(App* app, int delta) {
    auto& settings = app->getConfigManager().getSettings();
    int newVolume = settings.volume + delta;
    if (newVolume < 0) newVolume = 0;
    if (newVolume > 200) newVolume = 200;
    settings.volume = newVolume;
    app->getConfigManager().saveSettings();
}

void SettingsMenu::changeChannelHopDelay(App* app, int delta) {
    auto& settings = app->getConfigManager().getSettings();
    int newDelay = settings.channelHopDelayMs + delta;
    if (newDelay < 100) newDelay = 100;
    if (newDelay > 10000) newDelay = 10000;
    settings.channelHopDelayMs = newDelay;
    app->getConfigManager().saveSettings();
}

void SettingsMenu::changeKeyboardLayout(App* app, int direction) {
    auto& configManager = app->getConfigManager();
    auto& settings = configManager.getSettings();
    int numLayouts = configManager.getKeyboardLayouts().size();
    
    settings.keyboardLayoutIndex = (settings.keyboardLayoutIndex + direction + numLayouts) % numLayouts;
    configManager.saveSettings();
}

void SettingsMenu::draw(App* app, U8G2& display) {
    const int list_start_y = STATUS_BAR_H + 1;
    const int item_h = 18;

    display.setClipWindow(0, list_start_y, 127, 63);
    display.setFont(u8g2_font_6x10_tf);

    for (size_t i = 0; i < menuItems_.size(); ++i) {
        int item_center_y_rel = (int)animation_.itemOffsetY[i];
        float scale = animation_.itemScale[i];
        if (scale <= 0.01f) continue;
        
        int item_center_y_abs = (list_start_y + (63 - list_start_y) / 2) + item_center_y_rel;
        int item_top_y = item_center_y_abs - item_h / 2;

        if (item_top_y > 63 || item_top_y + item_h < list_start_y) continue;
        
        bool isSelected = (i == selectedIndex_);
        const auto& item = menuItems_[i];

        if (isSelected) {
            drawRndBox(display, 2, item_top_y, 124, item_h, 2, true);
            display.setDrawColor(0);
        } else {
            display.setDrawColor(1);
        }

        auto& configManager = app->getConfigManager();
        auto& settings = configManager.getSettings();
        
        display.drawStr(8, item_center_y_abs + 4, item.label.c_str());

        std::string valueText;
        if (item.label == "Brightness") {
            int percent = map(settings.mainDisplayBrightness, 0, 255, 0, 100);
            char buf[16];
            snprintf(buf, sizeof(buf), "< %d%% >", percent);
            valueText = buf;
        }
        else if (item.label == "Volume") {
            char buf[16];
            snprintf(buf, sizeof(buf), "< %d%% >", settings.volume);
            valueText = buf;
        }
        else if (item.label == "Hop Delay") {
            char buf[16];
            snprintf(buf, sizeof(buf), "< %dms >", settings.channelHopDelayMs);
            valueText = buf;
        }
        else if (item.label == "KB Layout") {
            const auto& layouts = configManager.getKeyboardLayouts();
            const std::string& layoutName = layouts[settings.keyboardLayoutIndex].first;
            valueText = "< " + layoutName + " >";
        }

        if (!valueText.empty()) {
            int text_w = display.getStrWidth(valueText.c_str());
            display.drawStr(128 - text_w - 8, item_center_y_abs + 4, valueText.c_str());
        }
    }
    
    display.setDrawColor(1);
    display.setMaxClipWindow();
}