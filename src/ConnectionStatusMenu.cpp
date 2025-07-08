#include "ConnectionStatusMenu.h"
#include "App.h"
#include "UI_Utils.h"
#include "WifiManager.h"

ConnectionStatusMenu::ConnectionStatusMenu() : entryTime_(0) {}

void ConnectionStatusMenu::onEnter(App* app) {
    entryTime_ = millis();
}

void ConnectionStatusMenu::onUpdate(App* app) {
    // --- MODIFIED: Use new navigation logic ---
    if (millis() - entryTime_ > 3000) {
        WifiListMenu* wifiMenu = static_cast<WifiListMenu*>(app->getMenu(MenuType::WIFI_LIST));
        if (wifiMenu) {
            wifiMenu->setScanOnEnter(false);
        }
        app->returnToMenu(MenuType::WIFI_LIST);
    }
}

void ConnectionStatusMenu::onExit(App* app) {}

void ConnectionStatusMenu::handleInput(App* app, InputEvent event) {
    // --- MODIFIED: Use new navigation logic ---
    if (event != InputEvent::NONE) {
        WifiListMenu* wifiMenu = static_cast<WifiListMenu*>(app->getMenu(MenuType::WIFI_LIST));
        if (wifiMenu) {
            wifiMenu->setScanOnEnter(false);
        }
        app->returnToMenu(MenuType::WIFI_LIST);
    }
}

void ConnectionStatusMenu::draw(App* app, U8G2& display) {
    WifiManager& wifi = app->getWifiManager();
    WifiState state = wifi.getState();
    String msg = wifi.getStatusMessage();

    IconType icon = IconType::INFO; 
    if (state == WifiState::CONNECTING) {
        msg = "Connecting";
        int dots = (millis() / 300) % 4;
        for (int i=0; i<dots; ++i) msg += ".";
    } else if (state == WifiState::CONNECTED) {
        icon = IconType::NET_WIFI;
    } else if (state == WifiState::CONNECTION_FAILED) {
        icon = IconType::NAV_BACK; 
    }
    
    display.setFont(u8g2_font_7x13B_tr);
    drawCustomIcon(display, display.getDisplayWidth()/2 - 8, 20, icon);
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(msg.c_str()))/2, 48, msg.c_str());
}