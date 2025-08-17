#include "HandshakeCaptureMenu.h"
#include "App.h"

HandshakeCaptureMenu::HandshakeCaptureMenu() : GridMenu("Handshake Sniffer", {
    MenuItem{"EAPOL (Scan)", IconType::NET_WIFI, MenuType::NONE,
        [](App* app) {
            app->getHandshakeCapture().prepare(HandshakeCaptureMode::EAPOL, HandshakeCaptureType::SCANNER);
            app->changeMenu(MenuType::HANDSHAKE_CAPTURE_ACTIVE);
        }},
    MenuItem{"EAPOL (Target)", IconType::TARGET, MenuType::NONE,
        [](App* app) {
            app->getHandshakeCapture().prepare(HandshakeCaptureMode::EAPOL, HandshakeCaptureType::TARGETED);
            app->getWifiListDataSource().setScanOnEnter(true);
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
            app->getWifiListDataSource().setScanOnEnter(true);
            app->changeMenu(MenuType::WIFI_LIST);
        }},
    MenuItem{"Back", IconType::NAV_BACK, MenuType::BACK}
}, 2) {}
