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
    // --- MODIFIED: Scan only if coming from a different menu ---
    // The navigation stack helps us know where we came from.
    const auto& navStack = app->getNavigationStack(); // Assuming App provides this getter
    bool cameFromTextInput = (navStack.size() > 1 && navStack[navStack.size() - 2] == MenuType::TEXT_INPUT);
    bool cameFromStatus = (navStack.size() > 1 && navStack[navStack.size() - 2] == MenuType::WIFI_CONNECTION_STATUS);

    if (cameFromTextInput || cameFromStatus) {
        // We are returning from a sub-menu, just rebuild the list with existing data
        rebuildDisplayItems(app);
    } else {
        // We are entering for the first time (e.g., from Settings), so start a scan.
        app->getWifiManager().startScan();
        displayItems_.clear(); // Clear old items while scanning
    }

    lastKnownState_ = app->getWifiManager().getState();
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
    
    WifiManager& wifi = app->getWifiManager();
    const auto& networks = wifi.getScannedNetworks();
    std::string connectedSsid = wifi.getCurrentSsid().c_str();
    
    // Find the connected network to move it to the top
    if (!connectedSsid.empty()) {
        for (size_t i = 0; i < networks.size(); ++i) {
            if (connectedSsid == networks[i].ssid) {
                // Add the connected network first with an asterisk
                displayItems_.push_back({"* " + std::string(networks[i].ssid), ListItemType::NETWORK, (int)i});
                break;
            }
        }
    }

    // Add the rest of the networks, skipping the one we already added
    for (size_t i = 0; i < networks.size(); ++i) {
        if (connectedSsid != networks[i].ssid) {
            displayItems_.push_back({networks[i].ssid, ListItemType::NETWORK, (int)i});
        }
    }
    
    // Add standard options
    displayItems_.push_back({"Scan Again", ListItemType::SCAN, -1});
    displayItems_.push_back({"Back", ListItemType::BACK, -1});
    
    // If a network is connected, select it by default
    selectedIndex_ = (displayItems_.size() > 0 && !connectedSsid.empty()) ? 0 : 0;

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
                        displayItems_.clear(); // Clear list to show "Scanning..."
                        break;
                    case ListItemType::BACK:
                        app->changeMenu(MenuType::BACK);
                        break;
                    case ListItemType::NETWORK:
                        const auto& netInfo = wifi.getScannedNetworks()[selectedItem.networkIndex];
                        
                        // If already connected, disconnect
                        if (netInfo.isConnected) {
                            wifi.disconnect();
                            rebuildDisplayItems(app); // Rebuild list to remove asterisk
                            break;
                        }

                        wifi.setSsidToConnect(netInfo.ssid);
                        
                        if (netInfo.isSecure) {
                            TextInputMenu& textMenu = app->getTextInputMenu();
                            textMenu.configure(
                                "Enter Password", 
                                [&](App* cb_app, const char* password) {
                                    cb_app->getWifiManager().connectWithPassword(password);
                                    cb_app->changeMenu(MenuType::WIFI_CONNECTION_STATUS);
                                },
                                true
                            );
                            app->changeMenu(MenuType::TEXT_INPUT);
                        } else {
                            wifi.connectOpen(netInfo.ssid);
                            app->changeMenu(MenuType::WIFI_CONNECTION_STATUS);
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
        const auto& item = displayItems_[i];
        
        // --- Draw Left-side Icon (Signal, Scan, Back) ---
        if (item.type == ListItemType::NETWORK) {
            const auto& netInfo = wifi.getScannedNetworks()[item.networkIndex];
            drawWifiSignal(display, content_x + 2, item_top_y + 5, netInfo.rssi);
        } else if (item.type == ListItemType::SCAN) {
            drawCustomIcon(display, content_x + 2, item_center_y_abs - 4, IconType::UI_REFRESH, IconRenderSize::SMALL);
        } else if (item.type == ListItemType::BACK) {
            drawCustomIcon(display, content_x + 2, item_center_y_abs - 4, IconType::NAV_BACK, IconRenderSize::SMALL);
        }
        
        // --- Draw Text and Right-side Lock Icon ---
        int text_x = content_x + 25;
        int text_y_base = item_center_y_abs + 4;
        const char* text = item.label.c_str();

        // Check if we need to draw a lock icon
        bool drawLock = false;
        if (item.type == ListItemType::NETWORK) {
            const auto& netInfo = wifi.getScannedNetworks()[item.networkIndex];
            if (netInfo.isSecure) {
                drawLock = true;
            }
        }
        
        // Calculate available width for text, accounting for the lock icon
        int lock_icon_width = 8;
        int text_w_avail = (128 - text_x - 4) - (drawLock ? lock_icon_width : 0);

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

        // Draw the lock icon on the far right if needed
        if (drawLock) {
            int lock_x = 128 - 4 - IconSize::SMALL_WIDTH;
            int lock_y = item_center_y_abs - (IconSize::SMALL_HEIGHT / 2);
            // Use the proper function call, not manual drawing
            drawCustomIcon(display, lock_x, lock_y, IconType::NET_WIFI_LOCK, IconRenderSize::SMALL);
        }
    }
    display.setDrawColor(1);
    display.setMaxClipWindow();
}

void WifiListMenu::drawWifiSignal(U8G2& display, int x, int y_icon_top, int8_t rssi) {
    int num_bars_to_draw = 0;
    if (rssi >= -55) num_bars_to_draw = 4;
    else if (rssi >= -80) num_bars_to_draw = 3;
    else if (rssi >= -90) num_bars_to_draw = 2;
    else if (rssi >= -100) num_bars_to_draw = 1;

    int bar_width = 2;
    int bar_spacing = 1;
    int first_bar_height = 2;
    int bar_height_increment = 2;
    // Calculate total height of the icon to align it properly from the top
    int max_icon_height = first_bar_height + (3 * bar_height_increment);

    for (int i = 0; i < 4; i++) {
        int current_bar_height = first_bar_height + i * bar_height_increment;
        int bar_x_position = x + i * (bar_width + bar_spacing);
        int y_pos_for_drawing_this_bar = y_icon_top + (max_icon_height - current_bar_height);
        
        if (i < num_bars_to_draw) {
            // Draw a filled bar for active signal strength
            display.drawBox(bar_x_position, y_pos_for_drawing_this_bar, bar_width, current_bar_height);
        } else {
            // Draw an empty frame for inactive bars
            display.drawFrame(bar_x_position, y_pos_for_drawing_this_bar, bar_width, current_bar_height);
        }
    }
}