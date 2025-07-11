#include "WifiListMenu.h"
#include "App.h"
#include "UI_Utils.h"
#include "TextInputMenu.h"

WifiListMenu::WifiListMenu() : selectedIndex_(0),
                               scanOnEnter_(true),
                               marqueeActive_(false),
                               marqueeScrollLeft_(true),
                               lastKnownScanCount_(0),
                               isScanning_(false)
{
}

void WifiListMenu::setScanOnEnter(bool scan)
{
    scanOnEnter_ = scan;
}

void WifiListMenu::onEnter(App* app) {
    WifiManager& wifi = app->getWifiManager();

    if (scanOnEnter_) {
        wifi.startScan();
        displayItems_.clear();
        selectedIndex_ = 0;
        isScanning_ = true;
        lastKnownScanCount_ = wifi.getScanCompletionCount();
    } else {
        rebuildDisplayItems(app);
        isScanning_ = false;
    }
    
    // This part was missing for the non-scanning entry path
    if (!isScanning_) {
        animation_.init();
        animation_.startIntro(selectedIndex_, displayItems_.size());
    }
}

void WifiListMenu::onUpdate(App* app) {
    uint32_t currentScanCount = app->getWifiManager().getScanCompletionCount();

    if (isScanning_ && currentScanCount > lastKnownScanCount_) {
        rebuildDisplayItems(app);
        animation_.init();
        animation_.startIntro(selectedIndex_, displayItems_.size());
        isScanning_ = false;
        lastKnownScanCount_ = currentScanCount; // Update the count
    }

    animation_.update();
}

void WifiListMenu::onExit(App* app) {
    scanOnEnter_ = true;
    isScanning_ = false;
    marqueeActive_ = false; // Ensure marquee is off on exit
}

void WifiListMenu::rebuildDisplayItems(App* app) {
    displayItems_.clear();
    
    WifiManager& wifi = app->getWifiManager();
    const auto& scannedNetworks = wifi.getScannedNetworks();
    String connectedSsid = wifi.getCurrentSsid();

    // --- FIX: Limit the total number of items to avoid buffer overflows in the animation system.
    const int MAX_TOTAL_ITEMS = MAX_ANIM_ITEMS; 
    
    // Stage 1: Add the connected network first, if it exists.
    if (!connectedSsid.isEmpty()) {
        bool foundInScan = false;
        for (size_t i = 0; i < scannedNetworks.size(); ++i) {
            if (String(scannedNetworks[i].ssid) == connectedSsid) {
                String label = "* " + connectedSsid;
                // FIX: Correctly initialize all members of DisplayItem
                DisplayItem item = {label.c_str(), ListItemType::NETWORK, scannedNetworks[i].rssi, scannedNetworks[i].isSecure, (int)i};
                displayItems_.push_back(item);
                foundInScan = true;
                break;
            }
        }
        if (!foundInScan) {
            String label = "* " + connectedSsid;
            // Use placeholder values for RSSI/secure, and -1 for index.
            DisplayItem item = {label.c_str(), ListItemType::NETWORK, -50, true, -1};
            displayItems_.push_back(item);
        }
    }

    // Stage 2: Add all other networks from the scan list, up to the limit.
    for (size_t i = 0; i < scannedNetworks.size(); ++i) {
        // Stop if we're about to exceed the total capacity (leaving room for Scan and Back).
        if (displayItems_.size() >= MAX_TOTAL_ITEMS - 2) {
            break;
        }
        
        // The crucial check to prevent duplicates.
        if (String(scannedNetworks[i].ssid) != connectedSsid) {
            // FIX: Correctly initialize all members of DisplayItem
            DisplayItem item = {scannedNetworks[i].ssid, ListItemType::NETWORK, scannedNetworks[i].rssi, scannedNetworks[i].isSecure, (int)i};
            displayItems_.push_back(item);
        }
    }
    
    // Stage 3: Add the permanent "Scan Again" and "Back" options.
    // Use dummy values for network-specific fields.
    DisplayItem scanItem = {"Scan Again", ListItemType::SCAN, 0, false, -1};
    displayItems_.push_back(scanItem);

    DisplayItem backItem = {"Back", ListItemType::BACK, 0, false, -1};
    displayItems_.push_back(backItem);
    
    // Reset selection to the top of the newly built list.
    selectedIndex_ = 0;
}


void WifiListMenu::handleInput(App* app, InputEvent event) {
    if (isScanning_) {
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
            if (selectedIndex_ >= (int)displayItems_.size()) break;
            const auto& selectedItem = displayItems_[selectedIndex_];
            WifiManager& wifi = app->getWifiManager();

            switch(selectedItem.type) {
                case ListItemType::SCAN:
                    setScanOnEnter(true);
                    onEnter(app);
                    break;
                case ListItemType::BACK:
                    app->changeMenu(MenuType::BACK);
                    break;
                case ListItemType::NETWORK:
                {
                    if (selectedItem.label[0] == '*') {
                        app->showPopUp("Disconnect?", selectedItem.label.substr(2), [this](App* app_cb) {
                            app_cb->getWifiManager().disconnect();
                            this->setScanOnEnter(false); // Don't rescan after disconnecting
                            this->rebuildDisplayItems(app_cb);
                            this->animation_.init();
                            this->animation_.startIntro(this->selectedIndex_, this->displayItems_.size());
                        });

                    } else {
                        // FIX: Use consistent member name 'originalNetworkIndex'
                        if (selectedItem.originalNetworkIndex < 0 || (size_t)selectedItem.originalNetworkIndex >= wifi.getScannedNetworks().size()) {
                            break;
                        }
                        const auto& netInfo = wifi.getScannedNetworks()[selectedItem.originalNetworkIndex];
                        wifi.setSsidToConnect(netInfo.ssid);

                        if (netInfo.isSecure) {
                            if (wifi.tryConnectKnown(netInfo.ssid)) {
                                app->changeMenu(MenuType::WIFI_CONNECTION_STATUS);
                            } else {
                                TextInputMenu& textMenu = app->getTextInputMenu();
                                textMenu.configure(
                                    "Enter Password",
                                    [&](App* cb_app, const char* password) {
                                        cb_app->getWifiManager().connectWithPassword(password);
                                        // The submit action in TextInputMenu already changes to the status screen
                                    },
                                    true
                                );
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
        break;
        case InputEvent::BTN_BACK_PRESS:
            app->changeMenu(MenuType::BACK);
            break;
        default:
            break;
    }
}

void WifiListMenu::scroll(int direction)
{
    if (displayItems_.empty())
        return;
    int oldIndex = selectedIndex_;
    selectedIndex_ += direction;
    if (selectedIndex_ < 0)
    {
        selectedIndex_ = displayItems_.size() - 1;
    }
    else if (selectedIndex_ >= (int)displayItems_.size())
    {
        selectedIndex_ = 0;
    }

    if (selectedIndex_ != oldIndex)
    {
        animation_.setTargets(selectedIndex_, displayItems_.size());
        marqueeActive_ = false;
        marqueeScrollLeft_ = true; // Reset marquee scroll direction
    }
}

void WifiListMenu::draw(App *app, U8G2 &display)
{
    const int list_start_y = STATUS_BAR_H + 1;
    const int item_h = 18;

    if (isScanning_) {
        const char *msg = "Scanning...";
        display.setFont(u8g2_font_6x10_tf);
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(msg)) / 2, 38, msg);
        return;
    }

    display.setClipWindow(0, list_start_y, 127, 63);
    display.setFont(u8g2_font_6x10_tf);

    for (size_t i = 0; i < displayItems_.size(); ++i)
    {
        // This check prevents the out-of-bounds access that causes the bug.
        // It's a redundant safety check now that rebuildDisplayItems is fixed, but it's good practice.
        if (i >= MAX_ANIM_ITEMS) break;
        
        int item_center_y_rel = (int)animation_.itemOffsetY[i];
        float scale = animation_.itemScale[i];
        if (scale <= 0.01f)
            continue;

        int item_center_y_abs = (list_start_y + (63 - list_start_y) / 2) + item_center_y_rel;
        int item_top_y = item_center_y_abs - item_h / 2;

        if (item_top_y > 63 || item_top_y + item_h < list_start_y)
            continue;

        bool isSelected = ((int)i == selectedIndex_);

        if (isSelected)
        {
            drawRndBox(display, 2, item_top_y, 124, item_h, 2, true);
            display.setDrawColor(0);
        }
        else
        {
            display.setDrawColor(1);
        }

        int content_x = 4;
        const auto &item = displayItems_[i];

        // FIX: Use consistent member name 'originalNetworkIndex'
        if (item.type == ListItemType::NETWORK && item.originalNetworkIndex != -1)
        {
            // Use the locally stored RSSI for efficiency instead of re-fetching
            drawWifiSignal(display, content_x + 2, item_top_y + 5, item.rssi);
        }
        else if (item.type == ListItemType::SCAN)
        {
            drawCustomIcon(display, content_x + 2, item_center_y_abs - 4, IconType::UI_REFRESH, IconRenderSize::SMALL);
        }
        else if (item.type == ListItemType::BACK)
        {
            drawCustomIcon(display, content_x + 2, item_center_y_abs - 4, IconType::NAV_BACK, IconRenderSize::SMALL);
        }

        int text_x = content_x + 15;
        int text_y_base = item_center_y_abs + 4;
        const char *text = item.label.c_str();

        bool drawLock = false;
        // FIX: Use consistent member name 'originalNetworkIndex' and local data
        if (item.type == ListItemType::NETWORK)
        {
            if (item.isSecure)
            {
                drawLock = true;
            }
        }

        int lock_icon_width = 8;
        int text_w_avail = (128 - text_x - 4) - (drawLock ? lock_icon_width : 0);

        if (isSelected)
        {
            updateMarquee(marqueeActive_, marqueePaused_, marqueeScrollLeft_, marqueePauseStartTime_, lastMarqueeTime_, marqueeOffset_, marqueeText_, marqueeTextLenPx_, text, text_w_avail, display);
            display.setClipWindow(text_x, item_top_y, text_x + text_w_avail, item_top_y + item_h);
            if (marqueeActive_)
            {
                display.drawStr(text_x + (int)marqueeOffset_, text_y_base, marqueeText_);
            }
            else
            {
                display.drawStr(text_x, text_y_base, text);
            }
            display.setMaxClipWindow(); // Reset clip window immediately
        }
        else
        {
            char *truncated = truncateText(text, text_w_avail, display);
            display.drawStr(text_x, text_y_base, truncated);
        }

        if (drawLock)
        {
            int lock_x = 128 - 4 - IconSize::SMALL_WIDTH;
            int lock_y = item_center_y_abs - (IconSize::SMALL_HEIGHT / 2);
            if (isSelected) display.setDrawColor(0); // Ensure icon color is correct on selected item
            drawCustomIcon(display, lock_x, lock_y, IconType::NET_WIFI_LOCK, IconRenderSize::SMALL);
            if (isSelected) display.setDrawColor(1);
        }
    }
    display.setDrawColor(1);
    display.setMaxClipWindow();
}

void WifiListMenu::drawWifiSignal(U8G2 &display, int x, int y_icon_top, int8_t rssi)
{
    int num_bars_to_draw = 0;
    if (rssi >= -55)
        num_bars_to_draw = 4;
    else if (rssi >= -70)
        num_bars_to_draw = 3;
    else if (rssi >= -85)
        num_bars_to_draw = 2;
    else if (rssi < -85)
        num_bars_to_draw = 1;
    
    // For invalid RSSI values
    if (rssi > 0) num_bars_to_draw = 0;

    int bar_width = 2;
    int bar_spacing = 1;
    int first_bar_height = 2;
    int bar_height_increment = 2;
    int max_icon_height = first_bar_height + (3 * bar_height_increment);

    for (int i = 0; i < 4; i++)
    {
        int current_bar_height = first_bar_height + i * bar_height_increment;
        int bar_x_position = x + i * (bar_width + bar_spacing);
        int y_pos_for_drawing_this_bar = y_icon_top + (max_icon_height - current_bar_height);

        if (i < num_bars_to_draw)
        {
             // Fill it if the signal is strong enough
            display.drawBox(bar_x_position, y_pos_for_drawing_this_bar, bar_width, current_bar_height);
        }
    }
}