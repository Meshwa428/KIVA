#include "WifiListDataSource.h"
#include "App.h"
#include "UI_Utils.h"
#include "ListMenu.h"
#include "TextInputMenu.h"
#include "Deauther.h"

WifiListDataSource::WifiListDataSource() {}

void WifiListDataSource::setScanOnEnter(bool scan) {
    scanOnEnter_ = scan;
}

void WifiListDataSource::setBackNavOverride(bool override) {
    backNavOverride_ = override;
}

void WifiListDataSource::forceReload(App* app, ListMenu* menu) {
    rebuildDisplayItems(app);
    menu->reloadData(app);
}

void WifiListDataSource::onEnter(App* app, ListMenu* menu, bool isForwardNav) {
    WifiManager& wifi = app->getWifiManager();
    if (isForwardNav && scanOnEnter_) {
        wifi.startScan();
        isScanning_ = true;
        lastKnownScanCount_ = wifi.getScanCompletionCount();
        displayItems_.clear();
    } else {
        rebuildDisplayItems(app);
        isScanning_ = false;
    }
}

void WifiListDataSource::onExit(App* app, ListMenu* menu) {
    scanOnEnter_ = true;
    isScanning_ = false;
    backNavOverride_ = false;
}

void WifiListDataSource::onUpdate(App* app, ListMenu* menu) {
    uint32_t currentScanCount = app->getWifiManager().getScanCompletionCount();

    if (isScanning_ && currentScanCount > lastKnownScanCount_) {
        isScanning_ = false;
        lastKnownScanCount_ = currentScanCount;
        rebuildDisplayItems(app);
        menu->reloadData(app);
    }
}

void WifiListDataSource::rebuildDisplayItems(App* app) {
    displayItems_.clear();
    
    WifiManager& wifi = app->getWifiManager();
    const auto& scannedNetworks = wifi.getScannedNetworks();
    String connectedSsid = wifi.getCurrentSsid();
    
    if (!connectedSsid.isEmpty()) {
        bool foundInScan = false;
        for (size_t i = 0; i < scannedNetworks.size(); ++i) {
            if (String(scannedNetworks[i].ssid) == connectedSsid) {
                String label = "* " + connectedSsid;
                displayItems_.push_back({label.c_str(), ListItemType::NETWORK, scannedNetworks[i].rssi, scannedNetworks[i].isSecure, (int)i});
                foundInScan = true;
                break;
            }
        }
        if (!foundInScan) {
            String label = "* " + connectedSsid;
            displayItems_.push_back({label.c_str(), ListItemType::NETWORK, -50, true, -1});
        }
    }

    for (size_t i = 0; i < scannedNetworks.size(); ++i) {
        // This was the line with the error. It's no longer needed because the vectors are dynamic.
        // if (displayItems_.size() >= MAX_ANIM_ITEMS - 2) break; 
        if (String(scannedNetworks[i].ssid) != connectedSsid) {
            displayItems_.push_back({scannedNetworks[i].ssid, ListItemType::NETWORK, scannedNetworks[i].rssi, scannedNetworks[i].isSecure, (int)i});
        }
    }
    
    displayItems_.push_back({"Scan Again", ListItemType::SCAN, 0, false, -1});
    displayItems_.push_back({"Back", ListItemType::BACK, 0, false, -1});
}

int WifiListDataSource::getNumberOfItems(App* app) {
    if (isScanning_) return 0;
    return displayItems_.size();
}

void WifiListDataSource::onItemSelected(App* app, ListMenu* menu, int index) {
    if (isScanning_ || index >= (int)displayItems_.size()) return;

    const auto& selectedItem = displayItems_[index];
    WifiManager& wifi = app->getWifiManager();

    switch(selectedItem.type) {
        case ListItemType::SCAN:
            setScanOnEnter(true);
            onEnter(app, menu, true);
            menu->reloadData(app);
            break;
        case ListItemType::BACK:
            app->changeMenu(MenuType::BACK);
            break;
        case ListItemType::NETWORK: {
            auto& deauther = app->getDeauther();
            auto& evilTwin = app->getEvilTwin();

            if (deauther.isAttackPending()) {
                const auto& netInfo = wifi.getScannedNetworks()[selectedItem.originalNetworkIndex];
                if (deauther.start(netInfo)) {
                    app->changeMenu(MenuType::DEAUTH_ACTIVE);
                } else {
                    app->showPopUp("Error", "Failed to start attack.", nullptr, "OK", "", true);
                }
                return;
            }

            if (evilTwin.isAttackPending()) {
                const auto& netInfo = wifi.getScannedNetworks()[selectedItem.originalNetworkIndex];
                if (evilTwin.start(netInfo)) {
                    app->changeMenu(MenuType::EVIL_TWIN_ACTIVE);
                } else {
                    app->showPopUp("Error", "Failed to start twin.", nullptr, "OK", "", true);
                }
                return;
            }
            
            if (selectedItem.label.rfind("* ", 0) == 0) {
                app->showPopUp("Disconnect?", selectedItem.label.substr(2), [this, app, menu](App* app_cb) {
                    app_cb->getWifiManager().disconnect();
                    this->setScanOnEnter(false); 
                    this->forceReload(app_cb, menu);
                }, "OK", "Cancel", true);
            } else {
                const auto& netInfo = wifi.getScannedNetworks()[selectedItem.originalNetworkIndex];
                wifi.setSsidToConnect(netInfo.ssid);

                if (netInfo.isSecure) {
                    if (wifi.tryConnectKnown(netInfo.ssid)) {
                        app->changeMenu(MenuType::WIFI_CONNECTION_STATUS);
                    } else {
                        TextInputMenu& textMenu = app->getTextInputMenu();
                        textMenu.configure("Enter Password",
                            // --- THIS IS THE FIX ---
                            // Use replaceMenu to swap TEXT_INPUT with the status screen in the navigation history.
                            [](App* cb_app, const char* password) {
                                cb_app->getWifiManager().connectWithPassword(password);
                                cb_app->replaceMenu(MenuType::WIFI_CONNECTION_STATUS); // <-- CHANGE THIS LINE
                            }, true);
                        app->changeMenu(MenuType::TEXT_INPUT);
                    }
                } else {
                    wifi.connectOpen(netInfo.ssid);
                    app->changeMenu(MenuType::WIFI_CONNECTION_STATUS);
                }
            }
            break;
        }
    }
}

bool WifiListDataSource::drawCustomEmptyMessage(App* app, U8G2& display) {
    if (isScanning_) {
        const char *msg = "Scanning...";
        display.setFont(u8g2_font_6x10_tf);
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(msg)) / 2, 38, msg);
        return true;
    }
    return false;
}

void WifiListDataSource::drawItem(App* app, U8G2& display, ListMenu* menu, int index, int x, int y, int w, int h, bool isSelected) {
    if (index >= displayItems_.size()) return;
    const auto &item = displayItems_[index];
    
    display.setDrawColor(isSelected ? 0 : 1);
    
    int content_x = x + 2;
    int item_center_y_abs = y + h / 2;
    
    if (item.type == ListItemType::NETWORK) {
        drawWifiSignal(display, content_x + 2, y + 5, item.rssi);
    } else if (item.type == ListItemType::SCAN) {
        drawCustomIcon(display, content_x + 2, item_center_y_abs - 4, IconType::UI_REFRESH, IconRenderSize::SMALL);
    } else if (item.type == ListItemType::BACK) {
        drawCustomIcon(display, content_x + 2, item_center_y_abs - 4, IconType::NAV_BACK, IconRenderSize::SMALL);
    }

    int text_x = content_x + 15;
    int text_y_base = item_center_y_abs + 4;
    const char *text = item.label.c_str();
    bool drawLock = (item.type == ListItemType::NETWORK && item.isSecure);
    int lock_icon_width = 8;
    int text_w_avail = (x + w) - text_x - 4 - (drawLock ? lock_icon_width : 0);

    menu->updateAndDrawText(display, text, text_x, text_y_base, text_w_avail, isSelected);

    if (drawLock) {
        int lock_x = (x + w) - 4 - IconSize::SMALL_WIDTH;
        int lock_y = item_center_y_abs - (IconSize::SMALL_HEIGHT / 2);
        drawCustomIcon(display, lock_x, lock_y, IconType::NET_WIFI_LOCK, IconRenderSize::SMALL);
    }
}

void WifiListDataSource::drawWifiSignal(U8G2& display, int x, int y_icon_top, int8_t rssi) {
    int num_bars_to_draw = 0;
    if (rssi >= -55) num_bars_to_draw = 4;
    else if (rssi >= -70) num_bars_to_draw = 3;
    else if (rssi >= -85) num_bars_to_draw = 2;
    else if (rssi < -85) num_bars_to_draw = 1;

    int bar_width = 2, bar_spacing = 1, first_bar_height = 2, bar_height_increment = 2;
    int max_icon_height = first_bar_height + (3 * bar_height_increment);

    for (int i = 0; i < 4; i++) {
        int current_bar_height = first_bar_height + i * bar_height_increment;
        int bar_x_position = x + i * (bar_width + bar_spacing);
        int y_pos_for_drawing_this_bar = y_icon_top + (max_icon_height - current_bar_height);
        if (i < num_bars_to_draw) {
            display.drawBox(bar_x_position, y_pos_for_drawing_this_bar, bar_width, current_bar_height);
        }
    }
}