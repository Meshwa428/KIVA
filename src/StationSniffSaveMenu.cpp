#include "StationSniffSaveMenu.h"
#include "App.h"
#include "StationSniffer.h"
#include "Event.h"
#include "EventDispatcher.h"
#include "UI_Utils.h"
#include "SdCardManager.h"
#include "Logger.h"

StationSniffSaveMenu::StationSniffSaveMenu() : lastClientCount_(0) {}

void StationSniffSaveMenu::setTarget(const WifiNetworkInfo& target) {
    target_ = target;
}

void StationSniffSaveMenu::onEnter(App* app, bool isForwardNav) {
    EventDispatcher::getInstance().subscribe(EventType::APP_INPUT, this);
    lastClientCount_ = 0;
    if (!app->getStationSniffer().start(target_)) {
        app->showPopUp("Error", "Failed to start sniffer.", [app](App* cb_app){
            EventDispatcher::getInstance().publish(NavigateBackEvent());
        }, "OK", "", false);
    }
}

void StationSniffSaveMenu::onUpdate(App* app) {
    app->requestRedraw(); // Keep screen live for count updates
}

void StationSniffSaveMenu::onExit(App* app) {
    EventDispatcher::getInstance().unsubscribe(EventType::APP_INPUT, this);
    
    // Save the file on exit
    const auto& stations = app->getStationSniffer().getFoundStations();
    app->getStationSniffer().stop();

    if (stations.empty()) {
        app->showPopUp("Info", "No clients found to save.", nullptr, "OK", "", true);
        return;
    }

    char bssidStr[18];
    sprintf(bssidStr, "%02X%02X%02X%02X%02X%02X", target_.bssid[0], target_.bssid[1], target_.bssid[2], target_.bssid[3], target_.bssid[4], target_.bssid[5]);
    char filePath[64];
    snprintf(filePath, sizeof(filePath), "%s/%s.txt", SD_ROOT::DATA_CAPTURES_STATION_LISTS, bssidStr);

    File file = SdCardManager::getInstance().openFileUncached(filePath, FILE_WRITE);
    if (!file) {
        LOG(LogLevel::ERROR, "SNIFF_SAVE", "Failed to create station list file: %s", filePath);
        app->showPopUp("Error", "Could not save file to SD.", nullptr, "OK", "", true);
        return;
    }

    for (const auto& station : stations) {
        char macStr[18];
        sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", station.mac[0], station.mac[1], station.mac[2], station.mac[3], station.mac[4], station.mac[5]);
        file.println(macStr);
    }
    file.close();

    char successMsg[64];
    snprintf(successMsg, sizeof(successMsg), "Saved %d clients to file.", stations.size());
    app->showPopUp("Success", successMsg, nullptr, "OK", "", true);
}

void StationSniffSaveMenu::handleInput(InputEvent event, App* app) {
    if (event == InputEvent::BTN_BACK_PRESS || event == InputEvent::BTN_OK_PRESS) {
        // onExit will handle the saving logic before navigating.
        EventDispatcher::getInstance().publish(NavigateBackEvent());
    }
}

void StationSniffSaveMenu::draw(App* app, U8G2& display) {
    const auto& stations = app->getStationSniffer().getFoundStations();

    display.setFont(u8g2_font_7x13B_tr);
    const char* titleMsg = "Sniff & Save";
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(titleMsg)) / 2, 22, titleMsg);
    
    display.setFont(u8g2_font_6x10_tf);
    
    std::string targetSsid = "Target: " + std::string(target_.ssid);
    char* truncated = truncateText(targetSsid.c_str(), 124, display);
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(truncated)) / 2, 36, truncated);

    char statusStr[32];
    snprintf(statusStr, sizeof(statusStr), "Clients Found: %d", stations.size());
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(statusStr)) / 2, 48, statusStr);

    display.setFont(u8g2_font_5x8_t_cyrillic);
    const char* instruction = "Press BACK to Save & Exit";
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(instruction)) / 2, 60, instruction);
}