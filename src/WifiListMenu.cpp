#include "WifiListMenu.h"
#include "App.h"
#include "UI_Utils.h"
#include "TextInputMenu.h"

WifiListMenu::WifiListMenu() :
    selectedIndex_(0),
    lastKnownState_(WifiState::OFF),
    marqueeActive_(false),
    marqueeScrollLeft_(true)
{
}

void WifiListMenu::onEnter(App* app) {
    app->getWifiManager().startScan();
    lastKnownState_ = app->getWifiManager().getState();
    displayItems_.clear(); // Clear old items while scanning
    selectedIndex_ = 0;
}

void WifiListMenu::onUpdate(App* app) {
    WifiState currentState = app->getWifiManager().getState();

    // Check if a scan just finished
    if (lastKnownState_ == WifiState::SCANNING && currentState != WifiState::SCANNING) {
        rebuildDisplayItems(app);
    }

    lastKnownState_ = currentState;
    animation_.update();
}

void WifiListMenu::onExit(App* app) {
    // Optional: could turn off wifi here if not connecting,
    // but better to let the App or WifiManager decide that.
}

void WifiListMenu::rebuildDisplayItems(App* app) {
    displayItems_.clear();
    const auto& networks = app->getWifiManager().getScannedNetworks();
    for (size_t i = 0; i < networks.size(); ++i) {
        displayItems_.push_back({networks[i].ssid, ListItemType::NETWORK, (int)i});
    }
    displayItems_.push_back({"Scan Again", ListItemType::SCAN, -1});
    displayItems_.push_back({"Back", ListItemType::BACK, -1});
    
    selectedIndex_ = 0;
    animation_.init();
    animation_.startIntro(selectedIndex_, displayItems_.size());
}


void WifiListMenu::handleInput(App* app, InputEvent event) {
    // Don't process input while scanning
    if (app->getWifiManager().getState() == WifiState::SCANNING) {
        if (event == InputEvent::BTN_BACK_PRESS) {
             app->changeMenu(MenuType::BACK);
        }
        return;
    }

    switch(event) {
        case InputEvent::ENCODER_CW:
        case InputEvent::BTN_DOWN_PRESS:
            scroll(1);
            break;
        case InputEvent::ENCODER_CCW:
        case InputEvent::BTN_UP_PRESS:
            scroll(-1);
            break;
        case InputEvent::BTN_ENCODER_PRESS:
        case InputEvent::BTN_OK_PRESS:
            {
                if (selectedIndex_ >= displayItems_.size()) break;
                const auto& selectedItem = displayItems_[selectedIndex_];
                WifiManager& wifi = app->getWifiManager();

                switch(selectedItem.type) {
                    case ListItemType::SCAN:
                        wifi.startScan();
                        break;
                    case ListItemType::BACK:
                        app->changeMenu(MenuType::BACK);
                        break;
                    case ListItemType::NETWORK:
                        const auto& netInfo = wifi.getScannedNetworks()[selectedItem.networkIndex];
                        wifi.setSsidToConnect(netInfo.ssid);

                        if (netInfo.isSecure) {
                            // Configure and switch to the text input menu
                            TextInputMenu& textMenu = app->getTextInputMenu();
                            textMenu.configure(
                                "Enter Password", 
                                // This C++ lambda is the callback function
                                [&](App* cb_app, const char* password) {
                                    cb_app->getWifiManager().connectWithPassword(password);
                                    // After submitting, go back to the list menu to see status
                                    cb_app->changeMenu(MenuType::WIFI_LIST);
                                },
                                true // Mask input for password
                            );
                            app->changeMenu(MenuType::TEXT_INPUT);
                        } else {
                            wifi.connectOpen(netInfo.ssid);
                        }
                        break;
                }
            }
            break;
        case InputEvent::BTN_BACK_PRESS:
            app->changeMenu(MenuType::BACK);
            break;
        default:
            break;
    }
}

void WifiListMenu::scroll(int direction) {
    if (displayItems_.empty()) return;
    int oldIndex = selectedIndex_;
    selectedIndex_ += direction;
    if (selectedIndex_ < 0) {
        selectedIndex_ = displayItems_.size() - 1;
    } else if (selectedIndex_ >= (int)displayItems_.size()) {
        selectedIndex_ = 0;
    }

    if (selectedIndex_ != oldIndex) {
        animation_.setTargets(selectedIndex_, displayItems_.size());
        marqueeActive_ = false;
    }
}

void WifiListMenu::draw(App* app, U8G2& display) {
    WifiManager& wifi = app->getWifiManager();
    WifiState currentState = wifi.getState();

    const int list_start_y = STATUS_BAR_H + 1;
    const int item_h = 18;

    if (currentState == WifiState::SCANNING) {
        const char* msg = "Scanning...";
        display.setFont(u8g2_font_6x10_tf);
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(msg))/2, 38, msg);
        return;
    }
    
    // Animate and draw the list
    display.setClipWindow(0, list_start_y, 127, 63);
    display.setFont(u8g2_font_6x10_tf);

    for (size_t i = 0; i < displayItems_.size(); ++i) {
        int item_center_y_rel = (int)animation_.itemOffsetY[i];
        float scale = animation_.itemScale[i];
        if (scale <= 0.01f) continue;

        int item_center_y_abs = (list_start_y + (63 - list_start_y) / 2) + item_center_y_rel;
        int item_top_y = item_center_y_abs - item_h / 2;

        if (item_top_y > 63 || item_top_y + item_h < list_start_y) continue;

        bool isSelected = (i == selectedIndex_);

        if (isSelected) {
            drawRndBox(display, 2, item_top_y, 124, item_h, 2, true);
            display.setDrawColor(0);
        } else {
            display.setDrawColor(1);
        }

        int content_x = 4;
        // Draw icon or signal strength
        if (displayItems_[i].type == ListItemType::NETWORK) {
            const auto& netInfo = wifi.getScannedNetworks()[displayItems_[i].networkIndex];
            drawWifiSignal(display, content_x + 2, item_center_y_abs, netInfo.rssi);
            if(netInfo.isSecure) {
                 drawCustomIcon(display, content_x + 15, item_center_y_abs - 4, IconType::TOOL_JAMMING, IconRenderSize::SMALL);
            }
        } else if (displayItems_[i].type == ListItemType::SCAN) {
            drawCustomIcon(display, content_x + 2, item_center_y_abs - 4, IconType::UI_REFRESH, IconRenderSize::SMALL);
        } else if (displayItems_[i].type == ListItemType::BACK) {
            drawCustomIcon(display, content_x + 2, item_center_y_abs - 4, IconType::NAV_BACK, IconRenderSize::SMALL);
        }
        
        // Draw text
        int text_x = content_x + 25;
        int text_y_base = item_center_y_abs + 4;
        int text_w_avail = 128 - text_x - 4;
        const char* text = displayItems_[i].label.c_str();

        if (isSelected) {
            updateMarquee(marqueeActive_, marqueePaused_, marqueeScrollLeft_, marqueePauseStartTime_, lastMarqueeTime_, marqueeOffset_, marqueeText_, marqueeTextLenPx_, text, text_w_avail, display);
            if (marqueeActive_) {
                display.drawStr(text_x + (int)marqueeOffset_, text_y_base, marqueeText_);
            } else {
                display.drawStr(text_x, text_y_base, text);
            }
        } else {
            char* truncated = truncateText(text, text_w_avail, display);
            display.drawStr(text_x, text_y_base, truncated);
        }
    }
    display.setDrawColor(1);
    display.setMaxClipWindow();
}

void WifiListMenu::drawWifiSignal(U8G2& display, int x, int y_center, int8_t rssi) {
    int num_bars = 0;
    if (rssi > -55) num_bars = 4;
    else if (rssi > -70) num_bars = 3;
    else if (rssi > -80) num_bars = 2;
    else if (rssi > -90) num_bars = 1;

    for (int i = 0; i < 4; ++i) {
        int bar_h = 2 + (i * 2);
        if (i < num_bars) {
            display.drawBox(x + i*3, y_center - bar_h/2, 2, bar_h);
        } else {
            display.drawFrame(x + i*3, y_center - bar_h/2, 2, bar_h);
        }
    }
}