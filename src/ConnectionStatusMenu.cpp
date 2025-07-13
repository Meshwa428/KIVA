
#include "ConnectionStatusMenu.h"
#include "App.h"
#include "UI_Utils.h"
#include "WifiManager.h"

ConnectionStatusMenu::ConnectionStatusMenu() : entryTime_(0) {}

void ConnectionStatusMenu::onEnter(App* app) {
    entryTime_ = millis();
}

void ConnectionStatusMenu::onUpdate(App* app) {
    WifiManager& wifi = app->getWifiManager();
    WifiState state = wifi.getState();

    // --- THE CORRECT LOGIC ---
    // On SUCCESS, return to the list automatically after a short delay.
    if (state == WifiState::CONNECTED && millis() - entryTime_ > 2000) {
        app->changeMenu(MenuType::BACK);
    }

    // On FAILURE, also return automatically after a longer delay.
    if (state == WifiState::CONNECTION_FAILED && millis() - entryTime_ > 4000) {
        app->changeMenu(MenuType::BACK);
    }
}

void ConnectionStatusMenu::onExit(App* app) {
    // Get the WifiListMenu instance
    WifiListMenu* wifiMenu = static_cast<WifiListMenu*>(app->getMenu(MenuType::WIFI_LIST));
    if (wifiMenu) {
        WifiManager& wifi = app->getWifiManager();
        if (wifi.getState() == WifiState::CONNECTED) {
            // --- CRITICAL FIX ---
            // If we successfully connected, we do NOT want to do a slow re-scan.
            // We just want to rebuild the list from the current data.
            wifiMenu->setScanOnEnter(false);
        } else {
            // If the connection failed, it's good practice to re-scan to get fresh data.
            wifiMenu->setScanOnEnter(true);
        }
    }
}

void ConnectionStatusMenu::handleInput(App* app, InputEvent event) {
    // Allow user to press back to dismiss the screen immediately.
    if (event != InputEvent::NONE) {
        app->changeMenu(MenuType::BACK);
    }
}

void ConnectionStatusMenu::draw(App* app, U8G2& display) {
    WifiManager& wifi = app->getWifiManager();
    WifiState state = wifi.getState();
    
    IconType icon = IconType::NET_WIFI;
    const char* top_text = "Connecting";
    String bottom_text = wifi.getSsidToConnect();

    if (state == WifiState::CONNECTING) {
        int dots = (millis() / 300) % 4;
        static char connecting_buf[16];
        strcpy(connecting_buf, "Connecting");
        for (int i=0; i<dots; ++i) strcat(connecting_buf, ".");
        top_text = connecting_buf;
    } else if (state == WifiState::CONNECTED) {
        icon = IconType::INFO;
        top_text = "Connected";
        bottom_text = wifi.getCurrentSsid();
    } else if (state == WifiState::CONNECTION_FAILED) {
        icon = IconType::ERROR; 
        top_text = wifi.getStatusMessage().c_str(); 
    }

    // Draw Icon
    drawCustomIcon(display, display.getDisplayWidth()/2 - 8, 16, icon);

    // Draw Top Text
    display.setFont(u8g2_font_7x13B_tr);
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(top_text))/2, 45, top_text);

    // Draw Bottom Text (SSID)
    if (!bottom_text.isEmpty()) {
        display.setFont(u8g2_font_6x10_tf);
        char* truncated = truncateText(bottom_text.c_str(), 124, display);
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(truncated))/2, 58, truncated);
    }
}