#include "App.h"
#include "Icons.h"
#include "UI_Utils.h"
#include "DebugUtils.h"
#include "Event.h"
#include <algorithm>
#include <cmath>
#include <SdCardManager.h>
#include <esp_task_wdt.h>
#include "Timezones.h"
#include "HardwareManager.h"
#include "WifiManager.h"
#include "OtaManager.h"
#include "Jammer.h"
#include "BeaconSpammer.h"
#include "Deauther.h"
#include "EvilPortal.h"
#include "ProbeSniffer.h"
#include "KarmaAttacker.h"
#include "HandshakeCapture.h"
#include "ProbeFlooder.h"
#include "BleSpammer.h"
#include "DuckyScriptRunner.h"
#include "BleManager.h"
#include "ConfigManager.h"
#include "MusicPlayer.h"
#include "MusicLibraryManager.h"
#include "GameAudio.h"
#include "StationSniffer.h"
#include "AssociationSleeper.h"
#include "BadMsgAttacker.h"
#include "RtcManager.h"
#include "SystemDataProvider.h"

App& App::getInstance() {
    static App instance;
    return instance;
}

void App::requestRedraw() {
    redrawRequested_ = true;
}

App::App() : 
    currentMenu_(nullptr),
    statusBarMarqueeTextLenPx_(0),
    statusBarMarqueeOffset_(0.0f),
    lastStatusBarMarqueeTime_(0),
    statusBarMarqueeActive_(false),
    statusBarMarqueePaused_(false),
    statusBarMarqueePauseStartTime_(0),
    statusBarMarqueeScrollLeft_(true),
    currentProgressBarFillPx_(0.0f),
    mainMenu_(),
    toolsMenu_("Tools", MenuType::TOOLS_CAROUSEL, {
        {"WIFI", IconType::WIFI, MenuType::WIFI_TOOLS_GRID},
        {"BLE", IconType::NET_BLUETOOTH, MenuType::BLE_TOOLS_GRID},
        {"NRF24 Jammer", IconType::TOOL_JAMMING, MenuType::NRF_JAMMER_GRID},
        {"Host/Other", IconType::TOOL_INJECTION, MenuType::HOST_OTHER_GRID},
        {"Back", IconType::NAV_BACK, MenuType::BACK}}),
    gamesMenu_("Games", MenuType::GAMES_CAROUSEL, {
        {"Snake", IconType::GAME_SNAKE, MenuType::SNAKE_GAME},
        {"Tetris", IconType::GAME_TETRIS, MenuType::NONE, 
            [](App* app) {
                app->showPopUp("Info", "Tetris is coming soon!", nullptr, "OK", "", true);
            }
        },
        {"Back", IconType::NAV_BACK, MenuType::BACK}}),
    
    wifiToolsMenu_("WiFi Tools", MenuType::WIFI_TOOLS_GRID, {
        {"Attacks", IconType::SKULL, MenuType::WIFI_ATTACKS_LIST},
        {"Sniff/Capture", IconType::TOOL_PROBE, MenuType::WIFI_SNIFF_LIST},
        {"Analyze", IconType::INFO, MenuType::NONE, 
            [](App* app) { LOG(LogLevel::INFO, "WIFI", "Analyze placeholder selected."); }
        },
        {"Back", IconType::NAV_BACK, MenuType::BACK}
    }, 2),
    bleToolsMenu_("BLE Tools", MenuType::BLE_TOOLS_GRID, {
        {"Attacks", IconType::SKULL, MenuType::BLE_ATTACKS_LIST},
        {"Sniff/Capture", IconType::TOOL_PROBE, MenuType::NONE,
            [](App* app) { LOG(LogLevel::INFO, "BLE", "Sniff placeholder selected."); }
        },
        {"Analyze", IconType::INFO, MenuType::NONE,
            [](App* app) { LOG(LogLevel::INFO, "BLE", "Analyze placeholder selected."); }
        },
        {"Back", IconType::NAV_BACK, MenuType::BACK}
    }, 2),
    nrfJammerMenu_("NRF24 Jammer", MenuType::NRF_JAMMER_GRID, {
        {"BLE Jam", IconType::NET_BLUETOOTH, MenuType::NONE,
            [](App *app) {
                auto *jammerMenu = static_cast<JammingActiveMenu *>(app->getMenu(MenuType::JAMMING_ACTIVE));
                if (jammerMenu) {
                    jammerMenu->setJammingModeToStart(JammingMode::BLE);
                    JammerConfig cfg; cfg.technique = JammingTechnique::NOISE_INJECTION;
                    jammerMenu->setJammingConfig(cfg);
                    EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::JAMMING_ACTIVE));
                }
            }},
        {"BT Classic", IconType::NET_BLUETOOTH, MenuType::NONE,
            [](App *app) {
                auto *jammerMenu = static_cast<JammingActiveMenu *>(app->getMenu(MenuType::JAMMING_ACTIVE));
                if (jammerMenu) {
                    jammerMenu->setJammingModeToStart(JammingMode::BT_CLASSIC);
                    JammerConfig cfg; cfg.technique = JammingTechnique::CONSTANT_CARRIER;
                    jammerMenu->setJammingConfig(cfg);
                    EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::JAMMING_ACTIVE));
                }
            }},
        {"WiFi Narrow", IconType::WIFI, MenuType::NONE,
            [](App *app) {
                auto *jammerMenu = static_cast<JammingActiveMenu *>(app->getMenu(MenuType::JAMMING_ACTIVE));
                if (jammerMenu) {
                    jammerMenu->setJammingModeToStart(JammingMode::WIFI_NARROWBAND);
                    JammerConfig cfg; cfg.technique = JammingTechnique::NOISE_INJECTION;
                    jammerMenu->setJammingConfig(cfg);
                    EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::JAMMING_ACTIVE));
                }
            }},
        {"Wide Spec", IconType::TOOL_JAMMING, MenuType::NONE,
            [](App *app) {
                auto *jammerMenu = static_cast<JammingActiveMenu *>(app->getMenu(MenuType::JAMMING_ACTIVE));
                if (jammerMenu) {
                    jammerMenu->setJammingModeToStart(JammingMode::WIDE_SPECTRUM);
                    JammerConfig cfg; cfg.technique = JammingTechnique::CONSTANT_CARRIER;
                    jammerMenu->setJammingConfig(cfg);
                    EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::JAMMING_ACTIVE));
                }
            }},
        {"Zigbee", IconType::TOOL_INJECTION, MenuType::NONE, 
            [](App *app) {
                auto *jammerMenu = static_cast<JammingActiveMenu *>(app->getMenu(MenuType::JAMMING_ACTIVE));
                if (jammerMenu) {
                    jammerMenu->setJammingModeToStart(JammingMode::ZIGBEE);
                    JammerConfig cfg; cfg.technique = JammingTechnique::NOISE_INJECTION;
                    jammerMenu->setJammingConfig(cfg);
                    EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::JAMMING_ACTIVE));
                }
            }},
        {"Custom Flood", IconType::TOOL_INJECTION, MenuType::CHANNEL_SELECTION},
        {"Back", IconType::NAV_BACK, MenuType::BACK}}, 2),
    hostOtherMenu_("Host / Other Tools", MenuType::HOST_OTHER_GRID, {
        {"USB Ducky", IconType::USB, MenuType::NONE, 
            [](App* app){
                app->getDuckyScriptListDataSource().setExecutionMode(DuckyScriptRunner::Mode::USB);
                EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::DUCKY_SCRIPT_LIST));
            }},
        {"BLE Ducky", IconType::NET_BLUETOOTH, MenuType::NONE, 
            [](App* app){
                app->getDuckyScriptListDataSource().setExecutionMode(DuckyScriptRunner::Mode::BLE);
                EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::DUCKY_SCRIPT_LIST));
            }},
        {"IR Blaster", IconType::UI_LASER, MenuType::NONE,
            [](App* app) { LOG(LogLevel::INFO, "HOST", "IR Blaster placeholder selected."); }
        },
        {"Back", IconType::NAV_BACK, MenuType::BACK}
    }, 2),
    beaconModeMenu_("Beacon Spam Mode", MenuType::BEACON_MODE_GRID, {
        {"Random", IconType::BEACON, MenuType::NONE, 
            [](App *app) {
                auto* menu = static_cast<BeaconSpamActiveMenu*>(app->getMenu(MenuType::BEACON_SPAM_ACTIVE));
                if (menu) {
                    menu->setAttackParameters(BeaconSsidMode::RANDOM);
                    EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::BEACON_SPAM_ACTIVE));
                }
            }
        },
        {"From Probes", IconType::UI_REFRESH, MenuType::NONE,
            [](App* app) {
                auto* menu = static_cast<BeaconSpamActiveMenu*>(app->getMenu(MenuType::BEACON_SPAM_ACTIVE));
                if (menu) {
                    if (!SdCardManager::getInstance().exists(SD_ROOT::DATA_PROBES_SSID_CUMULATIVE)) {
                        app->showPopUp("Error", "Run Probe Sniffer to create a list first.", nullptr, "OK", "", true);
                        return;
                    }
                    menu->setAttackParameters(BeaconSsidMode::FILE_BASED, SD_ROOT::DATA_PROBES_SSID_CUMULATIVE);
                    EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::BEACON_SPAM_ACTIVE));
                }
            }
        },
        {"From SD", IconType::SD_CARD, MenuType::BEACON_FILE_LIST},
        {"Back", IconType::NAV_BACK, MenuType::BACK}
    }, 2),
    deauthModeMenu_("Deauth Attack", MenuType::DEAUTH_MODE_GRID, {
        {"Normal", IconType::TARGET, MenuType::NONE, [](App* app){
            app->getDeauther().prepareAttack(DeauthAttackType::NORMAL);
            auto& ds = app->getWifiListDataSource();
            ds.setSelectionCallback([](App* app_cb, const WifiNetworkInfo& net){
                if (app_cb->getDeauther().start(net)) EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::DEAUTH_ACTIVE));
            });
            ds.setScanOnEnter(true);
            EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::WIFI_LIST));
        }},
        {"Evil Twin", IconType::SKULL, MenuType::NONE, [](App* app){
            app->getDeauther().prepareAttack(DeauthAttackType::EVIL_TWIN);
            auto& ds = app->getWifiListDataSource();
            ds.setSelectionCallback([](App* app_cb, const WifiNetworkInfo& net){
                if (app_cb->getDeauther().start(net)) EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::DEAUTH_ACTIVE));
            });
            ds.setScanOnEnter(true);
            EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::WIFI_LIST));
        }},
        {"Broadcast", IconType::TOOL_JAMMING, MenuType::NONE, [](App* app){
            app->getDeauther().prepareAttack(DeauthAttackType::BROADCAST_NORMAL);
            if (app->getDeauther().startBroadcast()) EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::DEAUTH_ACTIVE));
        }},
        {"Broadcast Twin", IconType::BEACON, MenuType::NONE, [](App* app){
            app->getDeauther().prepareAttack(DeauthAttackType::BROADCAST_EVIL_TWIN);
            if (app->getDeauther().startBroadcast()) EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::DEAUTH_ACTIVE));
        }},
        {"Pinpoint", IconType::TOOL_INJECTION, MenuType::NONE, [](App* app){
            // STEP 1: Go to WifiList to choose the TARGET AP
            auto& wifiDS = app->getWifiListDataSource();
            wifiDS.setSelectionCallback([](App* app_cb, const WifiNetworkInfo& net){
                // STEP 2: An AP was chosen. Now go to the StationList for that AP.
                auto& stationDS = app_cb->getStationListDataSource();
                stationDS.setMode(true, net); // Live scan mode for this AP
                stationDS.setAttackCallback([](App* attack_app, const StationInfo& client){
                    // STEP 3: A client was chosen. Stop the sniffer to release the RF lock.
                    attack_app->getStationSniffer().stop();

                    // Now, prepare and launch the pinpoint attack. The Deauther can now acquire the lock.
                    attack_app->getDeauther().prepareAttack(DeauthAttackType::PINPOINT_CLIENT);
                    if(attack_app->getDeauther().start(client)) {
                        EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::DEAUTH_ACTIVE));
                    }
                });
                EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::STATION_LIST));
            });
            wifiDS.setScanOnEnter(true);
            EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::WIFI_LIST));
        }},
        {"Back", IconType::NAV_BACK, MenuType::BACK}
    }, 2),
    probeFloodModeMenu_("Probe Flood Mode", MenuType::PROBE_FLOOD_MODE_GRID, {
        {"Random Flood", IconType::TOOL_JAMMING, MenuType::NONE, 
            [](App *app) {
                auto* menu = static_cast<ProbeFloodActiveMenu*>(app->getMenu(MenuType::PROBE_FLOOD_ACTIVE));
                if (menu) {
                    menu->setAttackParameters(ProbeFloodMode::RANDOM);
                    EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::PROBE_FLOOD_ACTIVE));
                }
            }
        },
        {"Pinpoint Flood", IconType::TARGET, MenuType::NONE,
            [](App* app) {
                auto& ds = app->getWifiListDataSource();
                ds.setSelectionCallback([](App* app_cb, const WifiNetworkInfo& net) {
                    auto* menu = static_cast<ProbeFloodActiveMenu*>(app_cb->getMenu(MenuType::PROBE_FLOOD_ACTIVE));
                    if (menu) {
                        menu->setAttackParameters(net);
                        EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::PROBE_FLOOD_ACTIVE));
                    }
                });
                ds.setScanOnEnter(true);
                EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::WIFI_LIST));
            }
        },
        {"Auto Random Flood", IconType::WIFI, MenuType::NONE,
            [](App *app) {
                auto* menu = static_cast<ProbeFloodActiveMenu*>(app->getMenu(MenuType::PROBE_FLOOD_ACTIVE));
                if (menu) {
                    menu->setAttackParameters(ProbeFloodMode::AUTO_BROADCAST_PINPOINT);
                    EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::PROBE_FLOOD_ACTIVE));
                }
            }
        },
        {"Auto Sniffed Flood", IconType::TOOL_PROBE, MenuType::NONE,
            [](App* app) {
                 auto* menu = static_cast<ProbeFloodActiveMenu*>(app->getMenu(MenuType::PROBE_FLOOD_ACTIVE));
                if (menu) {
                    if (!SdCardManager::getInstance().exists(SD_ROOT::DATA_PROBES_SSID_SESSION)) {
                        app->showPopUp("Error", "Run Probe Sniffer to create a list first.", nullptr, "OK", "", true);
                        return;
                    }
                    menu->setAttackParameters(ProbeFloodMode::AUTO_SNIFFED_PINPOINT);
                    EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::PROBE_FLOOD_ACTIVE));
                }
            }
        },
        {"Back", IconType::NAV_BACK, MenuType::BACK}
    }, 2),
    associationSleepModeMenu_("Assoc Sleep Attack", MenuType::ASSOCIATION_SLEEP_MODES_GRID, {
        {"Normal", IconType::TARGET, MenuType::NONE, [](App* app){
            auto& ds = app->getWifiListDataSource();
            ds.setSelectionCallback([](App* app_cb, const WifiNetworkInfo& net){
                if (app_cb->getAssociationSleeper().start(net)) EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::ASSOCIATION_SLEEP_ACTIVE));
            });
            ds.setScanOnEnter(true);
            EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::WIFI_LIST));
        }},
        {"Broadcast", IconType::TOOL_JAMMING, MenuType::NONE, [](App* app){
            if (app->getAssociationSleeper().start()) EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::ASSOCIATION_SLEEP_ACTIVE));
        }},
        {"Pinpoint", IconType::TOOL_INJECTION, MenuType::NONE, [](App* app){
            // Same multi-step navigation logic as Deauth Pinpoint
            auto& wifiDS = app->getWifiListDataSource();
            wifiDS.setSelectionCallback([](App* app_cb, const WifiNetworkInfo& net){
                auto& stationDS = app_cb->getStationListDataSource();
                stationDS.setMode(true, net);
                stationDS.setAttackCallback([](App* attack_app, const StationInfo& client){
                    // Stop the sniffer first to release the hardware lock
                    attack_app->getStationSniffer().stop();
                    // Now start the sleeper, which can acquire its own lock
                    if(attack_app->getAssociationSleeper().start(client)) {
                         EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::ASSOCIATION_SLEEP_ACTIVE));
                    }
                });
                EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::STATION_LIST));
            });
            wifiDS.setScanOnEnter(true);
            EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::WIFI_LIST));
        }},
        {"Back", IconType::NAV_BACK, MenuType::BACK}
    }, 2),
    badMsgModesMenu_("Bad Msg Attack", MenuType::BAD_MSG_MODES_GRID, {
        {"Target AP", IconType::TARGET, MenuType::NONE, [](App* app){
            app->getBadMsgAttacker().prepareAttack(BadMsgAttacker::AttackType::TARGET_AP);
            auto& ds = app->getWifiListDataSource();
            ds.setSelectionCallback([](App* app_cb, const WifiNetworkInfo& net){
                if (app_cb->getBadMsgAttacker().start(net)) {
                    EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::BAD_MSG_ACTIVE));
                }
            });
            ds.setScanOnEnter(true);
            EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::WIFI_LIST));
        }},
        {"Pinpoint Client", IconType::TOOL_INJECTION, MenuType::NONE, [](App* app){
            app->getBadMsgAttacker().prepareAttack(BadMsgAttacker::AttackType::PINPOINT_CLIENT);
            auto& wifiDS = app->getWifiListDataSource();
            wifiDS.setSelectionCallback([](App* app_cb, const WifiNetworkInfo& net){
                auto& stationDS = app_cb->getStationListDataSource();
                stationDS.setMode(true, net); // Live scan for this AP
                stationDS.setAttackCallback([](App* attack_app, const StationInfo& client){
                    attack_app->getStationSniffer().stop(); // Release lock for attacker
                    if(attack_app->getBadMsgAttacker().start(client)) {
                        EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::BAD_MSG_ACTIVE));
                    }
                });
                EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::STATION_LIST));
            });
            wifiDS.setScanOnEnter(true);
            EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::WIFI_LIST));
        }},
        {"Back", IconType::NAV_BACK, MenuType::BACK}
    }, 2),
    settingsGridMenu_("Settings", MenuType::SETTINGS_GRID, {
        {"UI & Interaction", IconType::SETTING_DISPLAY, MenuType::UI_SETTINGS_LIST},
        {"Hardware", IconType::SETTINGS, MenuType::HARDWARE_SETTINGS_LIST},
        {"Connectivity", IconType::WIFI, MenuType::CONNECTIVITY_SETTINGS_LIST},
        {"System", IconType::SETTING_SYSTEM, MenuType::SYSTEM_SETTINGS_LIST},
        {"Back", IconType::NAV_BACK, MenuType::BACK}
    }, 2),
    firmwareUpdateGrid_("Update", MenuType::FIRMWARE_UPDATE_GRID, {
        {"Web Update", IconType::FIRMWARE_UPDATE, MenuType::NONE,
            [](App *app) {
                if (app->getOtaManager().startWebUpdate()) EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::OTA_STATUS));
                else app->showPopUp("Error", "Failed to start Web AP.", nullptr, "OK", "", true);
            }
        },
        {"SD Card", IconType::SD_CARD, MenuType::FIRMWARE_LIST_SD}, 
        {"Basic OTA", IconType::BASIC_OTA, MenuType::NONE,
            [](App *app) { app->getOtaManager().startBasicOta(); }
        },
        {"Back", IconType::NAV_BACK, MenuType::BACK}
    }, 2),
    
    // --- Simple Menus (Default Constructed) ---
    usbDriveMenu_(),
    infoMenu_(),
    snakeGameMenu_(),
    brightnessMenu_(),
    textInputMenu_(), 
    connectionStatusMenu_(),
    popUpMenu_(),
    otaStatusMenu_(),
    channelSelectionMenu_(),
    
    // --- Active Screens (Default Constructed) ---
    jammingActiveMenu_(),
    beaconSpamActiveMenu_(),
    deauthActiveMenu_(),
    evilPortalActiveMenu_(),
    probeSnifferActiveMenu_(),
    karmaActiveMenu_(),
    handshakeCaptureMenu_(),
    probeFloodActiveMenu_(),
    handshakeCaptureActiveMenu_(),
    bleSpamActiveMenu_(),
    duckyScriptActiveMenu_(),
    nowPlayingMenu_(),
    associationSleepActiveMenu_(),
    badMsgActiveMenu_(),

    // --- Data Sources ---
    wifiListDataSource_(),
    firmwareListDataSource_(),
    beaconFileListDataSource_(),
    portalListDataSource_(),
    duckyScriptListDataSource_(),
    musicLibraryDataSource_(),
    songListDataSource_(),
    stationListDataSource_(),
    wifiAttacksDataSource_({
        {"Beacon Spam", IconType::BEACON, MenuType::BEACON_MODE_GRID},
        {"Deauth", IconType::DISCONNECT, MenuType::DEAUTH_MODE_GRID},
        {"Assoc Sleep", IconType::SKULL, MenuType::ASSOCIATION_SLEEP_MODES_GRID},
        {"Bad Msg", IconType::SKULL, MenuType::BAD_MSG_MODES_GRID},
        {"Evil Portal", IconType::SKULL, MenuType::PORTAL_LIST},
        {"Probe Flood", IconType::TOOL_PROBE, MenuType::PROBE_FLOOD_MODE_GRID},
        {"Karma Attack", IconType::TOOL_INJECTION, MenuType::KARMA_ACTIVE},
        {"Back", IconType::NAV_BACK, MenuType::BACK}
    }),
    wifiSniffDataSource_({
        {"Handshake Sniffer", IconType::WIFI_LOCK, MenuType::HANDSHAKE_CAPTURE_MENU},
        {"Probe Sniffer", IconType::TOOL_PROBE, MenuType::PROBE_ACTIVE},
        {"Station Sniffer", IconType::TARGET, MenuType::NONE, [](App* app){
            // This is for the "Sniff & Save" feature
            // For now, it just goes to the WifiList to select an AP to sniff
            auto& wifiDS = app->getWifiListDataSource();
            wifiDS.setSelectionCallback([](App* app_cb, const WifiNetworkInfo& net){
                // TODO: Implement the file saving logic here.
                // For now, it will just open the live station list.
                auto& stationDS = app_cb->getStationListDataSource();
                stationDS.setMode(true, net); // Live scan mode
                stationDS.setAttackCallback(nullptr); // No attack, just viewing
                EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::STATION_LIST));
            });
            wifiDS.setScanOnEnter(true);
            EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::WIFI_LIST));
        }},
        {"Back", IconType::NAV_BACK, MenuType::BACK}
    }),
    bleAttacksDataSource_({
        {"Apple Juice", IconType::BEACON, MenuType::NONE, [](App* app){
            auto* menu = static_cast<BleSpamActiveMenu*>(app->getMenu(MenuType::BLE_SPAM_ACTIVE));
            if(menu) { menu->setSpamModeToStart(BleSpamMode::APPLE_JUICE); EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::BLE_SPAM_ACTIVE)); }
        }},
        {"Sour Apple", IconType::BEACON, MenuType::NONE, [](App* app){
            auto* menu = static_cast<BleSpamActiveMenu*>(app->getMenu(MenuType::BLE_SPAM_ACTIVE));
            if(menu) { menu->setSpamModeToStart(BleSpamMode::SOUR_APPLE); EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::BLE_SPAM_ACTIVE)); }
        }},
        {"Android", IconType::BEACON, MenuType::NONE, [](App* app){
            auto* menu = static_cast<BleSpamActiveMenu*>(app->getMenu(MenuType::BLE_SPAM_ACTIVE));
            if(menu) { menu->setSpamModeToStart(BleSpamMode::GOOGLE); EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::BLE_SPAM_ACTIVE)); }
        }},
        {"Samsung", IconType::BEACON, MenuType::NONE, [](App* app){
            auto* menu = static_cast<BleSpamActiveMenu*>(app->getMenu(MenuType::BLE_SPAM_ACTIVE));
            if(menu) { menu->setSpamModeToStart(BleSpamMode::SAMSUNG); EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::BLE_SPAM_ACTIVE)); }
        }},
        {"Swift Pair", IconType::BEACON, MenuType::NONE, [](App* app){
            auto* menu = static_cast<BleSpamActiveMenu*>(app->getMenu(MenuType::BLE_SPAM_ACTIVE));
            if(menu) { menu->setSpamModeToStart(BleSpamMode::MICROSOFT); EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::BLE_SPAM_ACTIVE)); }
        }},
        {"Tutti-Frutti", IconType::SKULL, MenuType::NONE, [](App* app){
            auto* menu = static_cast<BleSpamActiveMenu*>(app->getMenu(MenuType::BLE_SPAM_ACTIVE));
            if(menu) { menu->setSpamModeToStart(BleSpamMode::ALL); EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::BLE_SPAM_ACTIVE)); }
        }},
        {"Back", IconType::NAV_BACK, MenuType::BACK}
    }),
    
    // --- START: REPLACEMENT OF SETTINGS DATA SOURCES ---
    uiSettingsDataSource_({
        MenuItem{
            "Brightness", IconType::SETTING_DISPLAY, MenuType::BRIGHTNESS_MENU, nullptr, true,
            [](App* app) -> std::string {
                int percent = map(app->getConfigManager().getSettings().mainDisplayBrightness, 0, 255, 0, 100);
                char buf[16]; snprintf(buf, sizeof(buf), "< %d%% >", percent); return std::string(buf);
            },
            [](App* app, int dir) {
                auto& settings = app->getConfigManager().getSettings();
                int newVal = settings.mainDisplayBrightness + (dir * 13);
                if (newVal < 0) newVal = 0; if (newVal > 255) newVal = 255;
                settings.mainDisplayBrightness = newVal;
                settings.auxDisplayBrightness = newVal; // Unified control
                app->getConfigManager().saveSettings();
            }
        },
        MenuItem{
            "KB Layout", IconType::USB, MenuType::NONE, nullptr, true,
            [](App* app) -> std::string {
                auto& cfg = app->getConfigManager();
                const auto& layouts = cfg.getKeyboardLayouts();
                int idx = cfg.getSettings().keyboardLayoutIndex;
                return "< " + layouts[idx].first + " >";
            },
            [](App* app, int dir) {
                auto& cfg = app->getConfigManager();
                auto& settings = cfg.getSettings();
                int numLayouts = cfg.getKeyboardLayouts().size();
                settings.keyboardLayoutIndex = (settings.keyboardLayoutIndex + dir + numLayouts) % numLayouts;
                cfg.saveSettings();
            }
        },
        {"Back", IconType::NAV_BACK, MenuType::BACK}
    }),
    hardwareSettingsDataSource_({
        MenuItem{
            "Volume", IconType::SETTING_SOUND, MenuType::NONE, nullptr, true,
            [](App* app) -> std::string {
                char buf[16]; snprintf(buf, sizeof(buf), "< %d%% >", app->getConfigManager().getSettings().volume); return std::string(buf);
            },
            [](App* app, int dir) {
                auto& settings = app->getConfigManager().getSettings();
                int newVol = settings.volume + (dir * 5);
                if (newVol < 0) newVol = 0; if (newVol > 200) newVol = 200;
                settings.volume = newVol;
                app->getConfigManager().saveSettings();
            }
        },
        MenuItem{
            "Vibration", IconType::UI_VIBRATION, MenuType::NONE, nullptr, true,
            [](App* app) -> std::string { return app->getHardwareManager().isVibrationOn() ? "< ON >" : "< OFF >"; },
            [](App* app, int dir) {
                auto& hw = app->getHardwareManager();
                hw.setVibration(!hw.isVibrationOn());
            }
        },
        MenuItem{
            "Laser", IconType::UI_LASER, MenuType::NONE, nullptr, true,
            [](App* app) -> std::string { return app->getHardwareManager().isLaserOn() ? "< ON >" : "< OFF >"; },
            [](App* app, int dir) {
                auto& hw = app->getHardwareManager();
                hw.setLaser(!hw.isLaserOn());
            }
        },
        {"Back", IconType::NAV_BACK, MenuType::BACK}
    }),
    connectivitySettingsDataSource_({
        {"WiFi Settings", IconType::WIFI, MenuType::WIFI_LIST, 
            [](App* app) {
                app->getWifiListDataSource().setSelectionCallback(nullptr);
                EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::WIFI_LIST));
            }
        },
        MenuItem{
            "Hop Delay", IconType::UI_REFRESH, MenuType::NONE, nullptr, true,
            [](App* app) -> std::string {
                int ms = app->getConfigManager().getSettings().channelHopDelayMs;
                float seconds = (float)ms / 1000.0f;
                char buf[16];
                snprintf(buf, sizeof(buf), "< %.1fs >", seconds); 
                return std::string(buf);
            },
            [](App* app, int dir) {
                auto& settings = app->getConfigManager().getSettings();
                int newDelay = settings.channelHopDelayMs + (dir * 50);
                if (newDelay < 50) newDelay = 50;
                if (newDelay > 5000) newDelay = 5000;
                settings.channelHopDelayMs = newDelay;
                app->getConfigManager().saveSettings();
            }
        },
        MenuItem{
            "Atk Cooldown", IconType::SKULL, MenuType::NONE, nullptr, true,
            [](App* app) -> std::string {
                int ms = app->getConfigManager().getSettings().attackCooldownMs;
                char buf[16];
                if (ms < 1000) {
                    float seconds = (float)ms / 1000.0f;
                    snprintf(buf, sizeof(buf), "< %.1fs >", seconds);
                } 
                else {
                    int seconds = ms / 1000;
                    snprintf(buf, sizeof(buf), "< %ds >", seconds);
                }
                return std::string(buf);
            },
            [](App* app, int dir) {
                auto& settings = app->getConfigManager().getSettings();
                int currentCooldown = settings.attackCooldownMs;
                int newCooldown = currentCooldown;

                const int minVal = 100;
                const int maxVal = 30000;
                const int smallStep = 100;
                const int largeStep = 1000;

                if (dir > 0) { // Increasing value
                    if (currentCooldown < 1000) newCooldown += smallStep;
                    else newCooldown += largeStep;
                } else { // Decreasing value
                    if (currentCooldown > 1000) newCooldown -= largeStep;
                    else newCooldown -= smallStep;
                }
                
                if (newCooldown < minVal) newCooldown = minVal;
                if (newCooldown > maxVal) newCooldown = maxVal;
                
                settings.attackCooldownMs = newCooldown;
                app->getConfigManager().saveSettings();
            }
        },
        {"OTA Password", IconType::FIRMWARE_UPDATE, MenuType::NONE, 
            [](App* app) {
                TextInputMenu& textMenu = app->getTextInputMenu();
                textMenu.configure("New OTA Password", 
                    [](App* cb_app, const char* new_password) {
                        if (strlen(new_password) > 0 && strlen(new_password) < Firmware::MIN_AP_PASSWORD_LEN) {
                            cb_app->showPopUp("Error", "Pass must be > 8 chars.", nullptr, "OK", "", true);
                        } else {
                            auto& settings = cb_app->getConfigManager().getSettings();
                            strlcpy(settings.otaPassword, new_password, sizeof(settings.otaPassword));
                            cb_app->getConfigManager().saveSettings();
                            EventDispatcher::getInstance().publish(ReplaceMenuEvent(MenuType::CONNECTIVITY_SETTINGS_LIST));
                            cb_app->showPopUp("Success", "Password Saved.", nullptr, "OK", "", true);
                        }
                    }, false, app->getConfigManager().getSettings().otaPassword
                );
                EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::TEXT_INPUT));
            }
        },
        // --- THIS IS THE MODIFIED HYBRID MENU ITEM ---
        MenuItem{
            "Timezone", IconType::SETTINGS, MenuType::TIMEZONE_LIST, 
            // Action to navigate to the list on OK press
            [](App* app) {
                EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::TIMEZONE_LIST));
            },
            true,
            [](App* app) -> std::string {
                const char* currentTzString = app->getConfigManager().getSettings().timezoneString;
                for (const auto& tz : app->getTimezoneListDataSource().getTimezones()) {
                    if (strcmp(currentTzString, tz.posixTzString) == 0) {
                        return std::string("< ") + tz.shortName + " >";
                    }
                }
                return "< Unknown >";
            },
            [](App* app, int dir) {
                const auto& tzList = app->getTimezoneListDataSource().getTimezones();
                if (tzList.empty()) return;

                auto& settings = app->getConfigManager().getSettings();
                const char* currentTzString = settings.timezoneString;
                
                int currentIndex = -1;
                for (size_t i = 0; i < tzList.size(); ++i) {
                    if (strcmp(currentTzString, tzList[i].posixTzString) == 0) {
                        currentIndex = i;
                        break;
                    }
                }
                
                if (currentIndex == -1) {
                    currentIndex = 0; // Absolute fallback
                }

                int newIndex = (currentIndex + dir + tzList.size()) % tzList.size();
                
                strncpy(settings.timezoneString, tzList[newIndex].posixTzString, sizeof(settings.timezoneString) - 1);
                settings.timezoneString[sizeof(settings.timezoneString) - 1] = '\0';
                app->getConfigManager().saveSettings();

                app->getRtcManager().setTimezone(settings.timezoneString);
                // Re-sync internal clock to apply new TZ
                app->getRtcManager().syncInternalClock();
                app->requestRedraw(); // Force an immediate redraw of new time
            } 
        },
        {"Back", IconType::NAV_BACK, MenuType::BACK}
    }),
    // --- END: REPLACEMENT OF SETTINGS DATA SOURCES ---
    systemSettingsDataSource_({
        {"Firmware Update", IconType::FIRMWARE_UPDATE, MenuType::FIRMWARE_UPDATE_GRID},
        {"USB Drive Mode", IconType::USB, MenuType::USB_DRIVE_MODE},
        {"Reload Settings", IconType::SD_CARD, MenuType::NONE, [](App* app) {
            if (app->getConfigManager().reloadFromSdCard()) app->showPopUp("Success", "Settings reloaded.", nullptr, "OK", "", true);
            else app->showPopUp("Error", "Load failed.", nullptr, "OK", "", true);
        }},
        {"System Info", IconType::INFO, MenuType::INFO_MENU},
        {"Back", IconType::NAV_BACK, MenuType::BACK}
    }),
    // --- ListMenu Instances ---
    wifiListMenu_("Wi-Fi Setup", MenuType::WIFI_LIST, &wifiListDataSource_),
    stationListMenu_("Select Client", MenuType::STATION_LIST, &stationListDataSource_),
    firmwareListMenu_("Update from SD", MenuType::FIRMWARE_LIST_SD, &firmwareListDataSource_),
    beaconFileListMenu_("Select File", MenuType::BEACON_FILE_LIST, &beaconFileListDataSource_),
    portalListMenu_("Portal", MenuType::PORTAL_LIST, &portalListDataSource_),
    duckyScriptListMenu_("Ducky Scripts", MenuType::DUCKY_SCRIPT_LIST, &duckyScriptListDataSource_),
    musicLibraryMenu_("Music Library", MenuType::MUSIC_LIBRARY, &musicLibraryDataSource_),
    songListMenu_("Songs", MenuType::SONG_LIST, &songListDataSource_),
    
    // New ListMenus using the generic source
    wifiAttacksMenu_("WiFi Attacks", MenuType::WIFI_ATTACKS_LIST, &wifiAttacksDataSource_),
    wifiSniffMenu_("Sniff/Capture", MenuType::WIFI_SNIFF_LIST, &wifiSniffDataSource_),
    bleAttacksMenu_("BLE Attacks", MenuType::BLE_ATTACKS_LIST, &bleAttacksDataSource_),
    uiSettingsMenu_("UI & Interaction", MenuType::UI_SETTINGS_LIST, &uiSettingsDataSource_),
    hardwareSettingsMenu_("Hardware", MenuType::HARDWARE_SETTINGS_LIST, &hardwareSettingsDataSource_),
    connectivitySettingsMenu_("Connectivity", MenuType::CONNECTIVITY_SETTINGS_LIST, &connectivitySettingsDataSource_),
    systemSettingsMenu_("System", MenuType::SYSTEM_SETTINGS_LIST, &systemSettingsDataSource_),
    timezoneMenu_("Timezone", MenuType::TIMEZONE_LIST, &timezoneDataSource_)
{
    for (int i = 0; i < MAX_LOG_LINES_SMALL_DISPLAY; ++i)
    {
        smallDisplayLogBuffer_[i][0] = '\0';
    }
}


void App::setup()
{
    Serial.begin(115200);
    delay(100);
    Wire.begin();

    serviceManager_ = std::make_unique<ServiceManager>(this);

    getHardwareManager().setAmplifier(false);
    getHardwareManager().getMainDisplay().begin();
    getHardwareManager().getMainDisplay().enableUTF8Print();
    getHardwareManager().getSmallDisplay().begin();
    getHardwareManager().getSmallDisplay().enableUTF8Print();

    logToSmallDisplay("Kiva Boot Agent v1.0", nullptr);

    struct BootTask {
        const char* name;
        std::function<bool()> action;
    };

    std::vector<BootTask> bootTasks = {
        {"Hardware Manager", [&](){ getHardwareManager().setup(this); return true; }},
        {"SD Card",          [&](){ return SdCardManager::getInstance().setup(); }},
        {"Config Manager",   [&](){ getConfigManager().setup(this); return true; }},
        {"RTC Manager",      [&](){ getRtcManager().setup(this); return true; }},
        {"System Data",      [&](){ getSystemDataProvider().setup(this); return true; }},
        {"Logger",           [&](){ Logger::getInstance().setup(); return true; }},
    };

    int totalTasks = bootTasks.size();
    U8G2 &mainDisplay = getHardwareManager().getMainDisplay();
    int progressBarDrawableWidth = mainDisplay.getDisplayWidth() - 40 - 2;

    for (int i = 0; i < totalTasks; ++i) {
        // --- ENHANCED LOGGING ---
        Serial.printf("[BOOT] Starting Task: %s\n", bootTasks[i].name);
        logToSmallDisplay(bootTasks[i].name, "INIT");
        
        bool success = bootTasks[i].action();

        if (success) {
            Serial.printf("[BOOT] > Task '%s' completed successfully.\n", bootTasks[i].name);
            logToSmallDisplay(bootTasks[i].name, "OK");
        } else {
            Serial.printf("[BOOT] > Task '%s' FAILED.\n", bootTasks[i].name);
            logToSmallDisplay(bootTasks[i].name, "FAIL");
        }
        
        float targetFillPx = (float)(i + 1) / totalTasks * progressBarDrawableWidth;
        while (currentProgressBarFillPx_ < targetFillPx - 0.5f) {
            float diff = targetFillPx - currentProgressBarFillPx_;
            currentProgressBarFillPx_ += diff * 0.2f;
            if (currentProgressBarFillPx_ > targetFillPx) currentProgressBarFillPx_ = targetFillPx;
            updateAndDrawBootScreen(0, 0);
            delay(10); 
        }
        currentProgressBarFillPx_ = targetFillPx;
        delay(30);
    }
    
    currentProgressBarFillPx_ = progressBarDrawableWidth;
    updateAndDrawBootScreen(0, 0);
    logToSmallDisplay("KivaOS Loading...");

    drawSecondaryDisplay();
    // delay(500);


    
    // Main Navigation
    menuRegistry_[MenuType::MAIN] = &mainMenu_;
    menuRegistry_[MenuType::TOOLS_CAROUSEL] = &toolsMenu_;
    menuRegistry_[MenuType::GAMES_CAROUSEL] = &gamesMenu_;

    menuRegistry_[MenuType::SNAKE_GAME] = &snakeGameMenu_;

    // Tools Sub-menus
    menuRegistry_[MenuType::WIFI_TOOLS_GRID] = &wifiToolsMenu_;
    menuRegistry_[MenuType::BLE_TOOLS_GRID] = &bleToolsMenu_;
    menuRegistry_[MenuType::NRF_JAMMER_GRID] = &nrfJammerMenu_;
    menuRegistry_[MenuType::HOST_OTHER_GRID] = &hostOtherMenu_;

    menuRegistry_[MenuType::WIFI_ATTACKS_LIST] = &wifiAttacksMenu_;
    menuRegistry_[MenuType::WIFI_SNIFF_LIST] = &wifiSniffMenu_;
    menuRegistry_[MenuType::BLE_ATTACKS_LIST] = &bleAttacksMenu_;
    
    menuRegistry_[MenuType::BEACON_MODE_GRID] = &beaconModeMenu_;
    menuRegistry_[MenuType::DEAUTH_MODE_GRID] = &deauthModeMenu_;
    menuRegistry_[MenuType::PROBE_FLOOD_MODE_GRID] = &probeFloodModeMenu_;
    menuRegistry_[MenuType::ASSOCIATION_SLEEP_MODES_GRID] = &associationSleepModeMenu_;
    menuRegistry_[MenuType::BAD_MSG_MODES_GRID] = &badMsgModesMenu_;
    menuRegistry_[MenuType::STATION_LIST] = &stationListMenu_;

    // Settings Sub-menus
    menuRegistry_[MenuType::SETTINGS_GRID] = &settingsGridMenu_;
    menuRegistry_[MenuType::UI_SETTINGS_LIST] = &uiSettingsMenu_;
    menuRegistry_[MenuType::HARDWARE_SETTINGS_LIST] = &hardwareSettingsMenu_;
    menuRegistry_[MenuType::CONNECTIVITY_SETTINGS_LIST] = &connectivitySettingsMenu_;
    menuRegistry_[MenuType::SYSTEM_SETTINGS_LIST] = &systemSettingsMenu_;
    menuRegistry_[MenuType::BRIGHTNESS_MENU] = &brightnessMenu_;
    menuRegistry_[MenuType::TIMEZONE_LIST] = &timezoneMenu_;

    // Core Tool Screens (Lists, etc.)
    menuRegistry_[MenuType::WIFI_LIST] = &wifiListMenu_;
    menuRegistry_[MenuType::BEACON_FILE_LIST] = &beaconFileListMenu_;
    menuRegistry_[MenuType::PORTAL_LIST] = &portalListMenu_;
    menuRegistry_[MenuType::DUCKY_SCRIPT_LIST] = &duckyScriptListMenu_;
    menuRegistry_[MenuType::CHANNEL_SELECTION] = &channelSelectionMenu_;
    menuRegistry_[MenuType::HANDSHAKE_CAPTURE_MENU] = &handshakeCaptureMenu_;
    menuRegistry_[MenuType::FIRMWARE_LIST_SD] = &firmwareListMenu_;
    
    // Active Screens
    menuRegistry_[MenuType::BEACON_SPAM_ACTIVE] = &beaconSpamActiveMenu_;
    menuRegistry_[MenuType::DEAUTH_ACTIVE] = &deauthActiveMenu_;
    menuRegistry_[MenuType::EVIL_PORTAL_ACTIVE] = &evilPortalActiveMenu_;
    menuRegistry_[MenuType::PROBE_FLOOD_ACTIVE] = &probeFloodActiveMenu_;
    menuRegistry_[MenuType::KARMA_ACTIVE] = &karmaActiveMenu_;
    menuRegistry_[MenuType::PROBE_ACTIVE] = &probeSnifferActiveMenu_;
    menuRegistry_[MenuType::HANDSHAKE_CAPTURE_ACTIVE] = &handshakeCaptureActiveMenu_;
    menuRegistry_[MenuType::BLE_SPAM_ACTIVE] = &bleSpamActiveMenu_;
    menuRegistry_[MenuType::DUCKY_SCRIPT_ACTIVE] = &duckyScriptActiveMenu_;
    menuRegistry_[MenuType::JAMMING_ACTIVE] = &jammingActiveMenu_;
    menuRegistry_[MenuType::ASSOCIATION_SLEEP_ACTIVE] = &associationSleepActiveMenu_;
    menuRegistry_[MenuType::BAD_MSG_ACTIVE] = &badMsgActiveMenu_;

    // Utilities / Misc
    menuRegistry_[MenuType::USB_DRIVE_MODE] = &usbDriveMenu_;
    menuRegistry_[MenuType::INFO_MENU] = &infoMenu_;
    menuRegistry_[MenuType::TEXT_INPUT] = &textInputMenu_;
    menuRegistry_[MenuType::POPUP] = &popUpMenu_;
    menuRegistry_[MenuType::FIRMWARE_UPDATE_GRID] = &firmwareUpdateGrid_;
    menuRegistry_[MenuType::OTA_STATUS] = &otaStatusMenu_;
    menuRegistry_[MenuType::WIFI_CONNECTION_STATUS] = &connectionStatusMenu_;
    
    // Music Player
    menuRegistry_[MenuType::MUSIC_LIBRARY] = &musicLibraryMenu_;
    menuRegistry_[MenuType::SONG_LIST] = &songListMenu_;
    menuRegistry_[MenuType::NOW_PLAYING] = &nowPlayingMenu_;

    // Subscribe to the events the App cares about
    EventDispatcher::getInstance().subscribe(EventType::NAVIGATE_TO_MENU, this);
    EventDispatcher::getInstance().subscribe(EventType::NAVIGATE_BACK, this);
    EventDispatcher::getInstance().subscribe(EventType::RETURN_TO_MENU, this);
    EventDispatcher::getInstance().subscribe(EventType::REPLACE_MENU, this);
    
    navigationStack_.clear();
    changeMenu(MenuType::MAIN, true);
    LOG(LogLevel::INFO, "App", "App setup finished.");
}

void App::loop()
{
    // 1. Update hardware and background services
    getHardwareManager().update();

    if (pendingMenuChange_ != MenuType::NONE) {
        changeMenu(pendingMenuChange_, isForwardNavPending_);
        pendingMenuChange_ = MenuType::NONE;
    } else if (backNavPending_) {
        changeMenu(MenuType::BACK, false);
        backNavPending_ = false;
    } else if (pendingReturnMenu_ != MenuType::NONE) {
        returnToMenu(pendingReturnMenu_);
        pendingReturnMenu_ = MenuType::NONE;
    } else if (pendingReplaceMenu_ != MenuType::NONE) {
        replaceMenu(pendingReplaceMenu_);
        pendingReplaceMenu_ = MenuType::NONE;
    }

    serviceManager_->loop();

    // 2. Let the current menu run its update logic (e.g., for animations)
    if (currentMenu_)
    {
        currentMenu_->onUpdate(this);
    }

    // UI pause check
    if (getHardwareManager().isUiRenderingPaused()) {
        delay(10); // Yield CPU time
        return;
    }

    // 3. Determine if a redraw is needed
    if (millis() - lastDrawTime_ > 1000) { // Periodic refresh for status bar time, etc.
        requestRedraw();
    }

    bool perfMode = getJammer().isActive() || getBeaconSpammer().isActive() || getDeauther().isActive() || 
                    getEvilPortal().isActive() || getKarmaAttacker().isAttacking() || getProbeFlooder().isActive() || getProbeSniffer().isActive() || getBleSpammer().isActive() ||
                    getDuckyRunner().isActive() || getAssociationSleeper().isActive();
    if (perfMode) {
        requestRedraw();
    }

    // 4. The core optimization: only draw if requested
    if (redrawRequested_) {
        redrawRequested_ = false;
        lastDrawTime_ = millis();

        // --- Drawing Logic ---
        U8G2 &mainDisplay = getHardwareManager().getMainDisplay();
        mainDisplay.clearBuffer();

        IMenu *menuForUI = currentMenu_;
        IMenu *underlyingMenu = nullptr;

        if (currentMenu_ && currentMenu_->getMenuType() == MenuType::POPUP) {
            underlyingMenu = getMenu(getPreviousMenuType());
            menuForUI = underlyingMenu; 
        }
        
        if (menuForUI) {
            if (!menuForUI->drawCustomStatusBar(this, mainDisplay)) {
                drawStatusBar();
            }
        } else if (currentMenu_) {
            drawStatusBar();
        }
        
        if (underlyingMenu) {
            underlyingMenu->draw(this, mainDisplay);
        }
        
        if (currentMenu_) {
            currentMenu_->draw(this, mainDisplay);
        }

        mainDisplay.sendBuffer();
        drawSecondaryDisplay();
        // --- End Drawing Logic ---

    } else {
        // --- Idle State ---
        delay(10); // Yield CPU time
    }

    // 5. Handle WiFi hardware state based on current needs
    bool wifiIsRequired = false; 
    if (currentMenu_)
    {
        MenuType currentType = currentMenu_->getMenuType();
        if (currentType == MenuType::WIFI_LIST ||
            currentType == MenuType::TEXT_INPUT ||
            currentType == MenuType::WIFI_CONNECTION_STATUS ||
            currentType == MenuType::ASSOCIATION_SLEEP_ACTIVE ||
            currentType == MenuType::BAD_MSG_ACTIVE ||
            currentType == MenuType::STATION_LIST)
        {
            wifiIsRequired = true;
        }
        else if (currentType == MenuType::OTA_STATUS && getOtaManager().getState() != OtaState::IDLE)
        {
            wifiIsRequired = true;
        }
    }
    
    if (getWifiManager().getState() == WifiState::CONNECTED || 
        getBeaconSpammer().isActive() ||
        getDeauther().isActive() || 
        getEvilPortal().isActive() || 
        getKarmaAttacker().isAttacking() || 
        getKarmaAttacker().isSniffing() ||
        getProbeFlooder().isActive() ||
        getProbeSniffer().isActive() ||
        getHandshakeCapture().isActive() ||
        getStationSniffer().isActive() ||
        getAssociationSleeper().isActive())
    {
        wifiIsRequired = true;
    }

    bool hostIsActive = getDuckyRunner().isActive();

    if (!wifiIsRequired && !hostIsActive && getWifiManager().isHardwareEnabled())
    {
        LOG(LogLevel::INFO, "App", "WiFi no longer required by any process. Disabling.");
        getWifiManager().setHardwareState(false);
    }
}

HardwareManager& App::getHardwareManager() { return *serviceManager_->getService<HardwareManager>(); }
WifiManager& App::getWifiManager() { return *serviceManager_->getService<WifiManager>(); }
OtaManager& App::getOtaManager() { return *serviceManager_->getService<OtaManager>(); }
Jammer& App::getJammer() { return *serviceManager_->getService<Jammer>(); }
BeaconSpammer& App::getBeaconSpammer() { return *serviceManager_->getService<BeaconSpammer>(); }
Deauther& App::getDeauther() { return *serviceManager_->getService<Deauther>(); }
EvilPortal& App::getEvilPortal() { return *serviceManager_->getService<EvilPortal>(); }
ProbeSniffer& App::getProbeSniffer() { return *serviceManager_->getService<ProbeSniffer>(); }
KarmaAttacker& App::getKarmaAttacker() { return *serviceManager_->getService<KarmaAttacker>(); }
HandshakeCapture& App::getHandshakeCapture() { return *serviceManager_->getService<HandshakeCapture>(); }
ProbeFlooder& App::getProbeFlooder() { return *serviceManager_->getService<ProbeFlooder>(); }
BleSpammer& App::getBleSpammer() { return *serviceManager_->getService<BleSpammer>(); }
DuckyScriptRunner& App::getDuckyRunner() { return *serviceManager_->getService<DuckyScriptRunner>(); }
MyBleManagerService& App::getBleManager() { return *serviceManager_->getService<MyBleManagerService>(); }
ConfigManager& App::getConfigManager() { return *serviceManager_->getService<ConfigManager>(); }
MusicPlayer& App::getMusicPlayer() { return *serviceManager_->getService<MusicPlayer>(); }
MusicLibraryManager& App::getMusicLibraryManager() { return *serviceManager_->getService<MusicLibraryManager>(); }
GameAudio& App::getGameAudio() { return *serviceManager_->getService<GameAudio>(); }
StationSniffer& App::getStationSniffer() { return *serviceManager_->getService<StationSniffer>(); }
AssociationSleeper& App::getAssociationSleeper() { return *serviceManager_->getService<AssociationSleeper>(); }
BadMsgAttacker& App::getBadMsgAttacker() { return *serviceManager_->getService<BadMsgAttacker>(); }
RtcManager& App::getRtcManager() { return *serviceManager_->getService<RtcManager>(); }
SystemDataProvider& App::getSystemDataProvider() { return *serviceManager_->getService<SystemDataProvider>(); }

TimezoneListDataSource& App::getTimezoneListDataSource() { return timezoneDataSource_; }
SongListDataSource& App::getSongListDataSource() { return songListDataSource_; }

const ConfigManager& App::getConfigManager() const { return *serviceManager_->getService<ConfigManager>(); }
const HardwareManager& App::getHardwareManager() const { return *serviceManager_->getService<HardwareManager>(); }
const RtcManager& App::getRtcManager() const { return *serviceManager_->getService<RtcManager>(); }
const SystemDataProvider& App::getSystemDataProvider() const { return *serviceManager_->getService<SystemDataProvider>(); }
const WifiManager& App::getWifiManager() const { return *serviceManager_->getService<WifiManager>(); }

void App::onEvent(const Event& event) {
    switch (event.type) {
        case EventType::NAVIGATE_TO_MENU: {
            const auto& navEvent = static_cast<const NavigateToMenuEvent&>(event);
            changeMenu(navEvent.menuType, navEvent.isForwardNav);
            break;
        }
        case EventType::NAVIGATE_BACK: {
            changeMenu(MenuType::BACK, false);
            break;
        }
        case EventType::RETURN_TO_MENU: {
            const auto& navEvent = static_cast<const ReturnToMenuEvent&>(event);
            returnToMenu(navEvent.menuType);
            break;
        }
        case EventType::REPLACE_MENU: {
            const auto& navEvent = static_cast<const ReplaceMenuEvent&>(event);
            replaceMenu(navEvent.menuType);
            break;
        }
        default:
            // App doesn't handle other events
            break;
    }
}

void App::changeMenu(MenuType type, bool isForwardNav) {
    // --- NEW: Reset status bar marquee whenever the menu changes ---
    statusBarMarqueeActive_ = false;
    statusBarMarqueeScrollLeft_ = true;

    // --- LOGGING POINT #2: MENU NAVIGATION ---
    const char* fromMenuName = (currentMenu_) ? DebugUtils::menuTypeToString(currentMenu_->getMenuType()) : "NULL";
    const char* toMenuName = DebugUtils::menuTypeToString(type);
    const char* navDirection = (isForwardNav) ? "Forward" : "Back";

    // Serial.printf("[NAV] Change: From '%s' -> To '%s' (%s)\n", fromMenuName, toMenuName, navDirection);
    LOG(LogLevel::INFO, "NAV", "Change: From '%s' -> To '%s' (%s)", fromMenuName, toMenuName, navDirection);

    if (type == MenuType::NONE) return;

    MenuType exitingMenuType = MenuType::NONE;
    if (currentMenu_) {
        exitingMenuType = currentMenu_->getMenuType();
    }

    if (type == MenuType::BACK)
    {
        if (navigationStack_.size() > 1)
        {
            navigationStack_.pop_back();
            // This is not recursive. Set the new type and isForwardNav flag for this call.
            type = navigationStack_.back();
            isForwardNav = false;
        }
        else
        {
            return; // Can't go back further
        }
    }

    // Check if the DESTINATION menu requires Wi-Fi.
    bool destinationRequiresWifi = (type == MenuType::WIFI_LIST ||
                                    type == MenuType::TEXT_INPUT ||
                                    type == MenuType::WIFI_CONNECTION_STATUS);

    // If the destination requires Wi-Fi and it's currently off, turn it on NOW.
    if (destinationRequiresWifi && !getWifiManager().isHardwareEnabled())
    {
        // Serial.printf("[APP-LOGIC] Destination menu (%d) requires WiFi. Enabling hardware...\n", (int)type);
        LOG(LogLevel::DEBUG, "App", "Destination menu (%s) requires WiFi. Enabling hardware...", toMenuName);
        
        getWifiManager().setHardwareState(true);
        delay(100); // Give hardware a moment to initialize before menu uses it.
    }

    auto it = menuRegistry_.find(type);
    if (it != menuRegistry_.end())
    {
        IMenu *newMenu = it->second;
        if (currentMenu_ == newMenu && !isForwardNav)
        {
            return;
        }

        if (currentMenu_)
        {
            currentMenu_->onExit(this);
        }
        currentMenu_ = newMenu;
        currentMenu_->onEnter(this, isForwardNav);

        if (isForwardNav)
        {
            if (exitingMenuType == MenuType::POPUP)
            {
                if (!navigationStack_.empty() && navigationStack_.back() == MenuType::POPUP)
                {
                    navigationStack_.pop_back();
                }
            }

            if (navigationStack_.empty() || navigationStack_.back() != type)
            {
                navigationStack_.push_back(type);
            }
        }
        requestRedraw();
    }
}

void App::replaceMenu(MenuType type) {
    LOG(LogLevel::INFO, "NAV", "Replace: From '%s' -> To '%s'", 
        (currentMenu_ ? DebugUtils::menuTypeToString(currentMenu_->getMenuType()) : "NULL"), 
        DebugUtils::menuTypeToString(type));

    // --- NEW: Reset status bar marquee ---
    statusBarMarqueeActive_ = false;
    statusBarMarqueeScrollLeft_ = true;

    if (currentMenu_) {
        currentMenu_->onExit(this);
    }
    
    // Pop the current menu from the stack
    if (!navigationStack_.empty()) {
        navigationStack_.pop_back();
    }
    
    // Find and set the new menu
    auto it = menuRegistry_.find(type);
    if (it != menuRegistry_.end()) {
        currentMenu_ = it->second;
        // Push the new menu onto the stack
        navigationStack_.push_back(type);
        // Enter the new menu as if it were a forward navigation
        currentMenu_->onEnter(this, true); 
        requestRedraw();
    }
}

void App::returnToMenu(MenuType type)
{
    while (!navigationStack_.empty() && navigationStack_.back() != type)
    {
        navigationStack_.pop_back();
    }

    if (navigationStack_.empty())
    {
        changeMenu(MenuType::MAIN, true);
        return;
    }

    changeMenu(navigationStack_.back(), false);
}

IMenu *App::getMenu(MenuType type)
{
    auto it = menuRegistry_.find(type);
    return (it != menuRegistry_.end()) ? it->second : nullptr;
}

void App::showPopUp(std::string title, std::string message, PopUpMenu::OnConfirmCallback onConfirm,
                    const std::string &confirmText, const std::string &cancelText, bool executeOnConfirmBeforeExit)
{
    popUpMenu_.configure(title, message, onConfirm, confirmText, cancelText, executeOnConfirmBeforeExit);
    changeMenu(MenuType::POPUP);
}

MenuType App::getPreviousMenuType() const
{
    if (navigationStack_.size() < 2)
    {
        return MenuType::MAIN;
    }
    // The previous menu is the one before the last element (which is the pop-up itself)
    return navigationStack_[navigationStack_.size() - 2];
}

void App::toggleSecondaryWidget(SecondaryWidgetType type) {
    uint32_t bit = 1 << static_cast<int>(type);
    uint32_t& mask = getConfigManager().getSettings().secondaryWidgetMask;
    bool is_active = (mask & bit) != 0;

    if (is_active) {
        mask &= ~bit; // Deactivate
    } else {
        // Count active widgets before activating a new one
        int active_count = 0;
        for (int i = 0; i < 5; ++i) { // Assuming 5 possible widget types
            if ((mask & (1 << i)) != 0) {
                active_count++;
            }
        }

        if (active_count >= 3) { // Limit is 3 user-selectable + 1 static (Voltage)
            showPopUp("Limit Reached", "Max 4 widgets allowed.", nullptr, "OK", "", true);
            return;
        }
        mask |= bit; // Activate
    }
    getConfigManager().saveSettings();
    requestRedraw();
}

bool App::isSecondaryWidgetActive(SecondaryWidgetType type) const {
    uint32_t bit = 1 << static_cast<int>(type);
    return (getConfigManager().getSettings().secondaryWidgetMask & bit) != 0;
}

std::vector<SecondaryWidgetType> App::getActiveSecondaryWidgets() const {
    std::vector<SecondaryWidgetType> active_widgets;
    for (int i = 0; i < 5; ++i) { // Assuming 5 possible widget types
        if ((getConfigManager().getSettings().secondaryWidgetMask & (1 << i)) != 0) {
            active_widgets.push_back(static_cast<SecondaryWidgetType>(i));
        }
    }
    return active_widgets;
}

void App::drawSecondaryDisplay()
{
    if (currentMenu_ && currentMenu_->getMenuType() == MenuType::TEXT_INPUT)
    {
        U8G2 &smallDisplay = getHardwareManager().getSmallDisplay();
        static_cast<TextInputMenu *>(currentMenu_)->draw(this, smallDisplay);
        return;
    }
    U8G2 &display = getHardwareManager().getSmallDisplay();
    display.clearBuffer();
    display.setDrawColor(1);

    // --- Top Bar ---
    display.setFont(u8g2_font_5x7_tf);
    // Time
    display.drawStr(2, 7, getRtcManager().getFormattedTime().c_str());

    // Battery
    char batPercentStr[5];
    snprintf(batPercentStr, sizeof(batPercentStr), "%d%%", getHardwareManager().getBatteryPercentage());
    int percentWidth = display.getStrWidth(batPercentStr);
    drawBatIcon(display, 128 - 12, 2, getHardwareManager().getBatteryPercentage());
    display.drawStr(128 - 12 - percentWidth - 2, 7, batPercentStr);

    // Status Icons
    int statusIconX = (128 - 18) / 2;
    if (getWifiManager().getState() == WifiState::CONNECTED) {
        drawCustomIcon(display, statusIconX, 1, IconType::WIFI, IconRenderSize::SMALL);
        statusIconX += 9;
    }
    if (getHardwareManager().isCharging()) {
        drawCustomIcon(display, statusIconX, 1, IconType::UI_CHARGING_BOLT, IconRenderSize::SMALL);
    }

    display.drawHLine(0, 9, 128);

    // --- Widgets ---
    std::vector<SecondaryWidgetType> active_widgets = getActiveSecondaryWidgets();
    std::vector<std::pair<std::string, std::string>> widget_data;

    // Always add Voltage
    char voltStr[8];
    snprintf(voltStr, sizeof(voltStr), "%.2fV", getHardwareManager().getBatteryVoltage());
    widget_data.push_back({"VOLT", voltStr});

    for (const auto& type : active_widgets) {
        char valStr[8];
        switch (type) {
            case SecondaryWidgetType::WIDGET_RAM:
                snprintf(valStr, sizeof(valStr), "%d%%", getSystemDataProvider().getRamUsage().percentage);
                widget_data.push_back({"RAM", valStr});
                break;
            case SecondaryWidgetType::WIDGET_PSRAM:
                snprintf(valStr, sizeof(valStr), "%d%%", getSystemDataProvider().getPsramUsage().percentage);
                widget_data.push_back({"PSRAM", valStr});
                break;
            case SecondaryWidgetType::WIDGET_SD:
                snprintf(valStr, sizeof(valStr), "%d%%", getSystemDataProvider().getSdCardUsage().percentage);
                widget_data.push_back({"SD", valStr});
                break;
            case SecondaryWidgetType::WIDGET_CPU:
                snprintf(valStr, sizeof(valStr), "%lu", getSystemDataProvider().getCpuFrequency());
                widget_data.push_back({"CPU", valStr});
                break;
            case SecondaryWidgetType::WIDGET_TEMP:
                snprintf(valStr, sizeof(valStr), "%.0fC", getSystemDataProvider().getTemperature());
                widget_data.push_back({"TEMP", valStr});
                break;
        }
    }

    int num_widgets = widget_data.size();
    if (num_widgets > 0) {
        int widget_w = (128 / num_widgets);
        for (int i = 0; i < num_widgets; ++i) {
            int widget_x = i * widget_w;
            display.setFont(u8g2_font_5x7_tf);
            display.drawStr(widget_x + (widget_w - display.getStrWidth(widget_data[i].first.c_str())) / 2, 18, widget_data[i].first.c_str());
            display.setFont(u8g2_font_6x10_tf);
            display.drawStr(widget_x + (widget_w - display.getStrWidth(widget_data[i].second.c_str())) / 2, 29, widget_data[i].second.c_str());
            if (i > 0) {
                display.drawVLine(widget_x, 11, 20);
            }
        }
    }

    display.sendBuffer();
}

void App::drawStatusBar()
{
    U8G2 &display = getHardwareManager().getMainDisplay();
    display.setFont(u8g2_font_6x10_tf);
    display.setDrawColor(1);

    const char *title = "KivaOS";
    if (currentMenu_ != nullptr)
    {
        title = currentMenu_->getTitle();
    }

    // --- Battery Drawing Logic ---
    int battery_area_x = 128 - 2;
    uint8_t batPercent = getHardwareManager().getBatteryPercentage();
    int bat_icon_width = 10;
    battery_area_x -= bat_icon_width;
    drawBatIcon(display, battery_area_x, 2, batPercent);

    // Charging Icon
    if (getHardwareManager().isCharging())
    {
        int charge_icon_width = 7;
        int charge_icon_spacing = 2;
        battery_area_x -= (charge_icon_width + charge_icon_spacing);
        drawCustomIcon(display, battery_area_x, 1, IconType::UI_CHARGING_BOLT, IconRenderSize::SMALL);
    }

    // Percentage Text
    char percentStr[5];
    snprintf(percentStr, sizeof(percentStr), "%d%%", batPercent);
    int percentWidth = display.getStrWidth(percentStr);
    int percent_text_spacing = 2;
    battery_area_x -= (percentWidth + percent_text_spacing);
    display.drawStr(battery_area_x, 8, percentStr);

    // --- NEW MARQUEE LOGIC FOR TITLE ---
    int title_x = 2;
    int title_y = 8;
    // Calculate available width based on where the battery info starts, with 4px padding
    int title_available_width = battery_area_x - title_x - 4;

    updateMarquee(statusBarMarqueeActive_, statusBarMarqueePaused_, statusBarMarqueeScrollLeft_, 
                  statusBarMarqueePauseStartTime_, lastStatusBarMarqueeTime_, statusBarMarqueeOffset_, 
                  statusBarMarqueeText_, sizeof(statusBarMarqueeText_), statusBarMarqueeTextLenPx_, title, title_available_width, display);

    // Clip the drawing area to prevent text from overflowing into the battery info
    display.setClipWindow(title_x, 0, title_x + title_available_width, STATUS_BAR_H);
    if (statusBarMarqueeActive_) {
        display.drawStr(title_x + (int)statusBarMarqueeOffset_, title_y, statusBarMarqueeText_);
    } else {
        display.drawStr(title_x, title_y, title);
    }
    display.setMaxClipWindow();

    // Bottom line
    display.drawLine(0, STATUS_BAR_H - 1, 127, STATUS_BAR_H - 1);
    this->requestRedraw();
}

void App::updateAndDrawBootScreen(unsigned long bootStartTime, unsigned long totalBootDuration)
{
    U8G2 &display = getHardwareManager().getMainDisplay(); // Explicitly get the main display

    display.clearBuffer();
    drawCustomIcon(display, 0, -4, IconType::BOOT_LOGO);

    int progressBarY = IconSize::BOOT_LOGO_HEIGHT - 12;
    int progressBarHeight = 7;
    int progressBarWidth = display.getDisplayWidth() - 40;
    int progressBarX = (display.getDisplayWidth() - progressBarWidth) / 2;
    int progressBarDrawableWidth = progressBarWidth - 2;

    display.setDrawColor(1);
    display.drawRFrame(progressBarX, progressBarY, progressBarWidth, progressBarHeight, 1);

    int fillWidthToDraw = (int)round(currentProgressBarFillPx_);
    fillWidthToDraw = std::max(0, std::min(fillWidthToDraw, progressBarDrawableWidth));

    if (fillWidthToDraw > 0)
    {
        display.drawRBox(progressBarX + 1, progressBarY + 1, fillWidthToDraw, progressBarHeight - 2, 0);
    }

    display.sendBuffer(); // Send the buffer to the main display
}

// This function is responsible for drawing logs on the SMALL display.
void App::logToSmallDisplay(const char *message, const char *status)
{
    U8G2 &display = getHardwareManager().getSmallDisplay(); // Explicitly get the small display
    display.setFont(u8g2_font_5x7_tf);

    char fullMessage[MAX_LOG_LINE_LENGTH_SMALL_DISPLAY];
    fullMessage[0] = '\0';
    int charsWritten = 0;

    if (status && strlen(status) > 0)
    {
        charsWritten = snprintf(fullMessage, sizeof(fullMessage), "[%s] ", status);
    }
    if (charsWritten < (int)sizeof(fullMessage) - 1)
    {
        snprintf(fullMessage + charsWritten, sizeof(fullMessage) - charsWritten, "%.*s",
                 (int)(sizeof(fullMessage) - charsWritten - 1), message);
    }

    // Scroll the log buffer up
    for (int i = 0; i < MAX_LOG_LINES_SMALL_DISPLAY - 1; ++i)
    {
        strncpy(smallDisplayLogBuffer_[i], smallDisplayLogBuffer_[i + 1], MAX_LOG_LINE_LENGTH_SMALL_DISPLAY);
    }

    // Add the new message at the bottom
    strncpy(smallDisplayLogBuffer_[MAX_LOG_LINES_SMALL_DISPLAY - 1], fullMessage, MAX_LOG_LINE_LENGTH_SMALL_DISPLAY);
    smallDisplayLogBuffer_[MAX_LOG_LINES_SMALL_DISPLAY - 1][MAX_LOG_LINE_LENGTH_SMALL_DISPLAY - 1] = '\0';

    display.clearBuffer();
    display.setDrawColor(1);
    const int lineHeight = 8;
    int yPos = display.getAscent();
    for (int i = 0; i < MAX_LOG_LINES_SMALL_DISPLAY; ++i)
    {
        display.drawStr(2, yPos + (i * lineHeight), smallDisplayLogBuffer_[i]);
    }
    display.sendBuffer(); // Send the buffer to the small display
}