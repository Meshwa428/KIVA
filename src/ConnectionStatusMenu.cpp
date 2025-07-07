#include "ConnectionStatusMenu.h"
#include "App.h"
#include "UI_Utils.h"
#include "WifiManager.h"

ConnectionStatusMenu::ConnectionStatusMenu() : entryTime_(0) {}

void ConnectionStatusMenu::onEnter(App* app) {
    entryTime_ = millis();
}

void ConnectionStatusMenu::onUpdate(App* app) {
    // After a few seconds, automatically go back to the Wi-Fi list
    if (millis() - entryTime_ > 3000) {
        app->changeMenu(MenuType::WIFI_LIST, false); // Go back, not forward
    }
}

void ConnectionStatusMenu::onExit(App* app) {}

void ConnectionStatusMenu::handleInput(App* app, InputEvent event) {
    // Any button press takes you back immediately
    if (event != InputEvent::NONE) {
        app->changeMenu(MenuType::WIFI_LIST, false);
    }
}

void ConnectionStatusMenu::draw(App* app, U8G2& display) {
    WifiManager& wifi = app->getWifiManager();
    WifiState state = wifi.getState();
    String msg = wifi.getStatusMessage();

    IconType icon = IconType::INFO; // Default icon
    if (state == WifiState::CONNECTING) {
        msg = "Connecting...";
        // Optional: add a spinner or dots animation here
        int dots = (millis() / 300) % 4;
        for (int i=0; i<dots; ++i) msg += ".";
    } else if (state == WifiState::CONNECTED) {
        icon = IconType::NET_WIFI;
    } else if (state == WifiState::CONNECTION_FAILED) {
        icon = IconType::NAV_BACK; // Using back as a 'fail' icon
    }
    
    display.setFont(u8g2_font_7x13B_tr);
    drawCustomIcon(display, display.getDisplayWidth()/2 - 8, 20, icon);
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(msg.c_str()))/2, 48, msg.c_str());
}