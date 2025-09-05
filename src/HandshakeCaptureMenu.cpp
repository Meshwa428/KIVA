#include "HandshakeCaptureMenu.h"
#include "App.h"

// --- THIS IS THE FIX ---
// Added 'MenuType::HANDSHAKE_CAPTURE_MENU' as the second argument
HandshakeCaptureMenu::HandshakeCaptureMenu() : GridMenu("Handshake Sniffer", MenuType::HANDSHAKE_CAPTURE_MENU, {
    MenuItem{"EAPOL (Scan)", IconType::NET_WIFI, MenuType::NONE,
        [](App* app) {
            app->getHandshakeCapture().prepare(HandshakeCaptureMode::EAPOL, HandshakeCaptureType::SCANNER);
            app->changeMenu(MenuType::HANDSHAKE_CAPTURE_ACTIVE);
        }},
    MenuItem{"EAPOL (Target)", IconType::TARGET, MenuType::NONE,
        [](App* app) {
            app->getHandshakeCapture().prepare(HandshakeCaptureMode::EAPOL, HandshakeCaptureType::TARGETED);
            
            auto& ds = app->getWifiListDataSource();
            ds.setSelectionCallback([](App* app_cb, const WifiNetworkInfo& selectedNetwork) {
                app_cb->getHandshakeCapture().start(selectedNetwork);
                app_cb->changeMenu(MenuType::HANDSHAKE_CAPTURE_ACTIVE);
            });

            ds.setScanOnEnter(true);
            app->changeMenu(MenuType::WIFI_LIST);
        }},
    MenuItem{"PMKID (Scan)", IconType::NET_WIFI, MenuType::NONE,
        [](App* app) {
            app->getHandshakeCapture().prepare(HandshakeCaptureMode::PMKID, HandshakeCaptureType::SCANNER);
            app->changeMenu(MenuType::HANDSHAKE_CAPTURE_ACTIVE);
        }},
    MenuItem{"PMKID (Target)", IconType::TARGET, MenuType::NONE,
        [](App* app) {
            app->getHandshakeCapture().prepare(HandshakeCaptureMode::PMKID, HandshakeCaptureType::TARGETED);

            auto& ds = app->getWifiListDataSource();
            ds.setSelectionCallback([](App* app_cb, const WifiNetworkInfo& selectedNetwork) {
                app_cb->getHandshakeCapture().start(selectedNetwork);
                app_cb->changeMenu(MenuType::HANDSHAKE_CAPTURE_ACTIVE);
            });

            ds.setScanOnEnter(true);
            app->changeMenu(MenuType::WIFI_LIST);
        }},
    MenuItem{"Back", IconType::NAV_BACK, MenuType::BACK}
}, 2) {}