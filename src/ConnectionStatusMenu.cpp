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

    // ONLY automatically return to the list if the connection was SUCCESSFUL.
    // This gives the user time to see the "Connected" message.
    if (wifi.getState() == WifiState::CONNECTED && millis() - entryTime_ > 4000) {
        WifiListMenu* wifiMenu = static_cast<WifiListMenu*>(app->getMenu(MenuType::WIFI_LIST));
        if (wifiMenu) {
            // Don't rescan after a successful connection, just show the updated list.
            wifiMenu->setScanOnEnter(false);
        }
        app->returnToMenu(MenuType::WIFI_LIST);
    }
}

void ConnectionStatusMenu::onExit(App* app) {
    // If the user backs out while we are in the CONNECTING state, we should cancel the attempt.
    WifiManager& wifi = app->getWifiManager();
    if (wifi.getState() == WifiState::CONNECTING) {
        wifi.disconnect();
    }
}

void ConnectionStatusMenu::handleInput(App* app, InputEvent event) {
    if (event == InputEvent::NONE) return;

    WifiManager& wifi = app->getWifiManager();
    WifiState state = wifi.getState();

    // If the connection has failed, ANY button press should dismiss the message and go back to the list.
    if (state == WifiState::CONNECTION_FAILED) {
        app->returnToMenu(MenuType::WIFI_LIST);
        return;
    }

    // Otherwise (if connecting or connected), any button press also goes back.
    // This allows the user to cancel a long connection attempt.
    WifiListMenu* wifiMenu = static_cast<WifiListMenu*>(app->getMenu(MenuType::WIFI_LIST));
    if (wifiMenu) {
        wifiMenu->setScanOnEnter(false);
    }
    app->returnToMenu(MenuType::WIFI_LIST);
}

void ConnectionStatusMenu::draw(App* app, U8G2& display) {
    WifiManager& wifi = app->getWifiManager();
    WifiState state = wifi.getState();
    
    IconType icon = IconType::UI_REFRESH; // Changed to a more "in-progress" icon
    const char* top_text = "Connecting";

    if (state == WifiState::CONNECTING) {
        int dots = (millis() / 300) % 4;
        static char connecting_buf[16];
        strcpy(connecting_buf, "Connecting");
        for (int i=0; i<dots; ++i) strcat(connecting_buf, ".");
        top_text = connecting_buf;
    } else if (state == WifiState::CONNECTED) {
        icon = IconType::NET_WIFI;
        top_text = "Connected";
    } else if (state == WifiState::CONNECTION_FAILED) {
        icon = IconType::ERROR; 
        top_text = wifi.getStatusMessage().c_str(); // Use the detailed message from WifiManager
    }

    drawCustomIcon(display, display.getDisplayWidth()/2 - 8, 16, icon);

    display.setFont(u8g2_font_7x13B_tr);
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(top_text))/2, 45, top_text);

    if (state == WifiState::CONNECTED) {
        std::vector<const uint8_t*> ssid_fonts = {
            u8g2_font_6x12_tr,
            u8g2_font_6x10_tf,
            u8g2_font_5x8_tf
        };
        int ssid_box_x = 4;
        int ssid_box_y = 52; // Lowered Y pos for better layout
        int ssid_box_w = 120;
        int ssid_box_h = 12; // Reduced height
        
        drawWrappedText(display, wifi.getCurrentSsid().c_str(), ssid_box_x, ssid_box_y, ssid_box_w, ssid_box_h, ssid_fonts);
    } else if (state == WifiState::CONNECTION_FAILED) {
        display.setFont(u8g2_font_5x7_tf);
        display.drawStr((display.getDisplayWidth() - display.getStrWidth("Press any key to return"))/2, 58, "Press any key to return");
    }
}