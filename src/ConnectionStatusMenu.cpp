
#include "Event.h"
#include "EventDispatcher.h"
#include "ConnectionStatusMenu.h"
#include "App.h"
#include "UI_Utils.h"
#include "WifiManager.h"

ConnectionStatusMenu::ConnectionStatusMenu() : entryTime_(0) {}

void ConnectionStatusMenu::onEnter(App* app, bool isForwardNav) {
    EventDispatcher::getInstance().subscribe(EventType::APP_INPUT, this);
    entryTime_ = millis();
}

void ConnectionStatusMenu::onUpdate(App* app) {
    WifiManager& wifi = app->getWifiManager();
    WifiState state = wifi.getState();

    // --- THE CORRECT LOGIC ---
    // On SUCCESS, return to the list automatically after a short delay.
    if (state == WifiState::CONNECTED && millis() - entryTime_ > 2000) {
        EventDispatcher::getInstance().publish(NavigateBackEvent());
    }

    // On FAILURE, also return automatically after a longer delay.
    if (state == WifiState::CONNECTION_FAILED && millis() - entryTime_ > 4000) {
        EventDispatcher::getInstance().publish(NavigateBackEvent());
    }
}

void ConnectionStatusMenu::onExit(App* app) {
    EventDispatcher::getInstance().unsubscribe(EventType::APP_INPUT, this);
    WifiListDataSource& wifiDataSource = app->getWifiListDataSource();
    WifiManager& wifi = app->getWifiManager();
    if (wifi.getState() == WifiState::CONNECTED) {
        wifiDataSource.setScanOnEnter(false);
    } else {
        wifiDataSource.setScanOnEnter(true);
    }
}

void ConnectionStatusMenu::handleInput(InputEvent event, App* app) {
    // Allow user to press back to dismiss the screen immediately.
    if (event != InputEvent::NONE) {
        EventDispatcher::getInstance().publish(NavigateBackEvent());
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
