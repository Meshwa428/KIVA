#include "WifiListMenu.h"
#include "App.h"
#include "UI_Utils.h"
#include "TextInputMenu.h"

WifiListMenu::WifiListMenu() : selectedIndex_(0),
                               scanOnEnter_(true), // Default to scan on entry
                               marqueeActive_(false),
                               marqueeScrollLeft_(true),
                               lastKnownScanCount_(0), // <-- INITIALIZE THIS
                               isScanning_(false)      // <-- INITIALIZE THIS
{
}

void WifiListMenu::setScanOnEnter(bool scan)
{
    scanOnEnter_ = scan;
}

void WifiListMenu::onEnter(App* app) {
    Serial.println("\n--- WifiListMenu::onEnter ---");
    Serial.printf("Flag 'scanOnEnter_' is currently: %s\n", scanOnEnter_ ? "true" : "false");

    WifiManager& wifi = app->getWifiManager();

    if (scanOnEnter_) {
        // App::changeMenu has already ensured hardware is ON. We can safely start the scan.
        Serial.println("Action: Requesting a NEW scan from WifiManager.");
        wifi.startScan();
        displayItems_.clear();
        selectedIndex_ = 0;
        isScanning_ = true;
        lastKnownScanCount_ = wifi.getScanCompletionCount();
    } else {
        Serial.println("Action: Skipping scan, rebuilding list from existing data.");
        rebuildDisplayItems(app);
        isScanning_ = false;
    }
    
    // This is for logging/debugging to confirm the state.
    Serial.printf("State at exit of onEnter: [WifiManager State: %d] [Hardware Enabled: %s]\n", 
                  (int)wifi.getState(), wifi.isHardwareEnabled() ? "true" : "false");
    Serial.println("-----------------------------\n");
}

void WifiListMenu::onUpdate(App* app) {
    uint32_t currentScanCount = app->getWifiManager().getScanCompletionCount();

    // Check if a scan we initiated has completed.
    if (isScanning_ && currentScanCount > lastKnownScanCount_) {
        Serial.println("\n--- WifiListMenu::onUpdate ---");
        Serial.println("Detected that a scan has just finished.");
        Serial.println("Action: Calling rebuildDisplayItems().");
        rebuildDisplayItems(app);
        
        // This is crucial: re-initialize the animation after the list is rebuilt.
        animation_.init();
        animation_.startIntro(selectedIndex_, displayItems_.size());
        
        isScanning_ = false; // Reset our local flag.
        Serial.println("----------------------------\n");
    }

    animation_.update();
}

void WifiListMenu::onExit(App* app) {
    // When leaving this menu, reset the flag to its default state
    // so the next time it's entered from the main flow, it scans.
    scanOnEnter_ = true;
    isScanning_ = false; // Ensure this is false on exit
}

void WifiListMenu::rebuildDisplayItems(App* app) {
    Serial.println("\n--- WifiListMenu::rebuildDisplayItems ---");
    displayItems_.clear();
    
    WifiManager& wifi = app->getWifiManager();
    const auto& scannedNetworks = wifi.getScannedNetworks();
    
    // Get the ground truth about the current connection from the manager.
    String connectedSsid = wifi.getCurrentSsid();
    Serial.printf("Data: `connectedSsid` from manager is: '%s'\n", connectedSsid.c_str());
    Serial.printf("Data: `scannedNetworks` list contains %d items.\n", scannedNetworks.size());
    
    // Stage 1: Add the connected network first if it exists.
    if (!connectedSsid.isEmpty()) {
        bool foundInScan = false;
        for (size_t i = 0; i < scannedNetworks.size(); ++i) {
            if (String(scannedNetworks[i].ssid) == connectedSsid) {
                // Found it. Create the item with a '*' and its original index from the scan.
                String label = "* " + connectedSsid;
                DisplayItem item = {label.c_str(), ListItemType::NETWORK, (int)i};
                displayItems_.push_back(item);
                foundInScan = true;
                Serial.printf("Action: Found connected SSID '%s' in scan results. Adding with '*'.\n", connectedSsid.c_str());
                break;
            }
        }
        // If the connected network is hidden (it wasn't in the scan), we must add it manually.
        if (!foundInScan) {
            String label = "* " + connectedSsid;
            DisplayItem item = {label.c_str(), ListItemType::NETWORK, -1};
            displayItems_.push_back(item);
            Serial.printf("Action: Connected SSID '%s' was NOT in scan results. Adding manually with '*'.\n", connectedSsid.c_str());
        }
    } else {
        Serial.println("Data: `connectedSsid` is empty. Not searching for a connected network.");
    }

    // Stage 2: Add all other networks from the scan list that are NOT the connected one.
    for (size_t i = 0; i < scannedNetworks.size(); ++i) {
        if (String(scannedNetworks[i].ssid) != connectedSsid) {
            DisplayItem item = {scannedNetworks[i].ssid, ListItemType::NETWORK, (int)i};
            displayItems_.push_back(item);
        }
    }
    
    // Stage 3: Add the permanent "Scan Again" and "Back" options.
    DisplayItem scanItem = {"Scan Again", ListItemType::SCAN, -1};
    displayItems_.push_back(scanItem);

    DisplayItem backItem = {"Back", ListItemType::BACK, -1};
    displayItems_.push_back(backItem);
    
    selectedIndex_ = 0;
    Serial.printf("Result: Rebuilt list now has %d items.\n", displayItems_.size());
    Serial.println("-----------------------------------------\n");
}

void WifiListMenu::handleInput(App* app, InputEvent event) {
    // Don't process input while scanning, except for the back button
    if (isScanning_) { // Use our local flag now
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
                    // This will now trigger the scan logic in onEnter correctly
                    setScanOnEnter(true);
                    onEnter(app);
                    break;
                case ListItemType::BACK:
                    app->changeMenu(MenuType::BACK);
                    break;
                case ListItemType::NETWORK:
                {
                    if (selectedItem.label[0] == '*') {
                        // --- UPDATED to use showPopUp ---
                        app->showPopUp("Disconnect?", selectedItem.label.substr(2), [this](App* app_cb) {
                            app_cb->getWifiManager().disconnect();
                            this->setScanOnEnter(false);
                            this->rebuildDisplayItems(app_cb);
                            this->animation_.init();
                            this->animation_.startIntro(this->selectedIndex_, this->displayItems_.size());
                        });

                    } else {
                        // This block remains unchanged
                        if (selectedItem.networkIndex < 0 || (size_t)selectedItem.networkIndex >= wifi.getScannedNetworks().size()) {
                            break;
                        }
                        const auto& netInfo = wifi.getScannedNetworks()[selectedItem.networkIndex];
                        wifi.setSsidToConnect(netInfo.ssid);
                        if (netInfo.isSecure) {
                            TextInputMenu& textMenu = app->getTextInputMenu();
                            textMenu.configure(
                                "Enter Password",
                                [&](App* cb_app, const char* password) {
                                    cb_app->getWifiManager().connectWithPassword(password);
                                },
                                true
                            );
                            app->changeMenu(MenuType::TEXT_INPUT);
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
    }
}

void WifiListMenu::draw(App *app, U8G2 &display)
{
    const int list_start_y = STATUS_BAR_H + 1;
    const int item_h = 18;

    if (isScanning_) { // Use our local flag now
        const char *msg = "Scanning...";
        display.setFont(u8g2_font_6x10_tf);
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(msg)) / 2, 38, msg);
        return;
    }

    display.setClipWindow(0, list_start_y, 127, 63);
    display.setFont(u8g2_font_6x10_tf);

    for (size_t i = 0; i < displayItems_.size(); ++i)
    {
        int item_center_y_rel = (int)animation_.itemOffsetY[i];
        float scale = animation_.itemScale[i];
        if (scale <= 0.01f)
            continue;

        int item_center_y_abs = (list_start_y + (63 - list_start_y) / 2) + item_center_y_rel;
        int item_top_y = item_center_y_abs - item_h / 2;

        if (item_top_y > 63 || item_top_y + item_h < list_start_y)
            continue;

        bool isSelected = (i == selectedIndex_);

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

        // This check needs to be safe if the item is for a hidden network
        if (item.type == ListItemType::NETWORK && item.networkIndex != -1)
        {
            const auto &netInfo = app->getWifiManager().getScannedNetworks()[item.networkIndex];
            drawWifiSignal(display, content_x + 2, item_top_y + 5, netInfo.rssi);
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
        // This check needs to be safe if the item is for a hidden network
        if (item.type == ListItemType::NETWORK && item.networkIndex != -1)
        {
            const auto &netInfo = app->getWifiManager().getScannedNetworks()[item.networkIndex];
            if (netInfo.isSecure)
            {
                drawLock = true;
            }
        }

        int lock_icon_width = 8;
        int text_w_avail = (128 - text_x - 4) - (drawLock ? lock_icon_width : 0);

        if (isSelected)
        {
            updateMarquee(marqueeActive_, marqueePaused_, marqueeScrollLeft_, marqueePauseStartTime_, lastMarqueeTime_, marqueeOffset_, marqueeText_, marqueeTextLenPx_, text, text_w_avail, display);
            if (marqueeActive_)
            {
                display.drawStr(text_x + (int)marqueeOffset_, text_y_base, marqueeText_);
            }
            else
            {
                display.drawStr(text_x, text_y_base, text);
            }
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
            drawCustomIcon(display, lock_x, lock_y, IconType::NET_WIFI_LOCK, IconRenderSize::SMALL);
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
    else if (rssi >= -80)
        num_bars_to_draw = 3;
    else if (rssi >= -90)
        num_bars_to_draw = 2;
    else if (rssi >= -100)
        num_bars_to_draw = 1;

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
            display.drawBox(bar_x_position, y_pos_for_drawing_this_bar, bar_width, current_bar_height);
        }
        else
        {
            display.drawFrame(bar_x_position, y_pos_for_drawing_this_bar, bar_width, current_bar_height);
        }
    }
}