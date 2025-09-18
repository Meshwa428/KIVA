#include "StationListDataSource.h"
#include "App.h"
#include "ListMenu.h"
#include "UI_Utils.h"
#include "SdCardManager.h"
#include "Logger.h"

StationListDataSource::StationListDataSource() : 
    isLiveScan_(true),
    attackCallback_(nullptr),
    lastKnownStationCount_(0)
{}

void StationListDataSource::setMode(bool liveScan, const WifiNetworkInfo& target, const std::string& filePath) {
    isLiveScan_ = liveScan;
    targetAp_ = target;
    stationsFilePath_ = filePath;
}

void StationListDataSource::setAttackCallback(AttackCallback callback) {
    attackCallback_ = callback;
}

void StationListDataSource::onEnter(App* app, ListMenu* menu, bool isForwardNav) {
    stations_.clear();
    lastKnownStationCount_ = 0;

    if (isLiveScan_) {
        if (!app->getStationSniffer().start(targetAp_)) {
            app->showPopUp("Error", "Failed to start sniffer.", [app](App* cb_app){
                EventDispatcher::getInstance().publish(NavigateBackEvent());
            }, "OK", "", false);
        }
    } else {
        loadStationsFromFile();
        menu->reloadData(app);
    }
}

void StationListDataSource::onExit(App* app, ListMenu* menu) {
    if (isLiveScan_) {
        app->getStationSniffer().stop();
    }
    attackCallback_ = nullptr;
}

void StationListDataSource::onUpdate(App* app, ListMenu* menu) {
    if (isLiveScan_) {
        const auto& foundStations = app->getStationSniffer().getFoundStations();
        if (foundStations.size() != lastKnownStationCount_) {
            stations_ = foundStations;
            lastKnownStationCount_ = stations_.size();
            menu->reloadData(app, false);
        }
    }
}

void StationListDataSource::loadStationsFromFile() {
    // Implement parsing logic for your station file format (e.g., CSV)
    // For now, this is a placeholder.
    LOG(LogLevel::INFO, "STA_DS", "Loading stations from %s", stationsFilePath_.c_str());
    // ... file parsing logic here ...
}

int StationListDataSource::getNumberOfItems(App* app) {
    return stations_.size();
}

void StationListDataSource::onItemSelected(App* app, ListMenu* menu, int index) {
    if (index >= stations_.size() || !attackCallback_) return;
    attackCallback_(app, stations_[index]);
}

bool StationListDataSource::drawCustomEmptyMessage(App* app, U8G2& display) {
    const char* msg = isLiveScan_ ? "Sniffing client..." : "0 clients found.";
    display.setFont(u8g2_font_6x10_tf);
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(msg)) / 2, 38, msg);
    return true;
}

void StationListDataSource::drawItem(App* app, U8G2& display, ListMenu* menu, int index, int x, int y, int w, int h, bool isSelected) {
    if (index >= stations_.size()) return;
    const auto& station = stations_[index];
    
    display.setDrawColor(isSelected ? 0 : 1);

    char macStr[18];
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
            station.mac[0], station.mac[1], station.mac[2],
            station.mac[3], station.mac[4], station.mac[5]);

    drawCustomIcon(display, x + 4, y + (h - IconSize::LARGE_HEIGHT) / 2, IconType::TARGET);
    int text_x = x + 4 + IconSize::LARGE_WIDTH + 4;
    int text_y = y + h / 2 + 4;
    int text_w = w - (text_x - x) - 4;
    
    menu->updateAndDrawText(display, macStr, text_x, text_y, text_w, isSelected);
}