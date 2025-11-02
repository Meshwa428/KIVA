#include "StationFileListDataSource.h"
#include "App.h"
#include "SdCardManager.h"
#include "ListMenu.h"
#include "UI_Utils.h"
#include "Event.h"
#include "EventDispatcher.h"
#include "Logger.h"
#include "BadMsgAttacker.h" // <-- FIX: Include the full definition of the class here.

void StationFileListDataSource::setTargetAp(const WifiNetworkInfo& target) {
    targetAp_ = target;
}

void StationFileListDataSource::onEnter(App* app, ListMenu* menu, bool isForwardNav) {
    fileNames_.clear();
    filePaths_.clear();
    const char* dirPath = SD_ROOT::DATA_CAPTURES_STATION_LISTS;
    
    File root = SdCardManager::getInstance().openFileUncached(dirPath);
    if (!root || !root.isDirectory()) {
        LOG(LogLevel::WARN, "STA_FILE_DS", "Station list directory not found.");
        return;
    }

    File file = root.openNextFile();
    while(file) {
        if (!file.isDirectory() && String(file.name()).endsWith(".txt")) {
            filePaths_.push_back(file.path());
            fileNames_.push_back(file.name());
        }
        file.close();
        file = root.openNextFile();
    }
    root.close();
}

void StationFileListDataSource::onExit(App* app, ListMenu* menu) {}

int StationFileListDataSource::getNumberOfItems(App* app) {
    return fileNames_.size();
}

void StationFileListDataSource::onItemSelected(App* app, ListMenu* menu, int index) {
    if (index >= filePaths_.size()) return;
    
    app->getBadMsgAttacker().prepareAttack(BadMsgAttacker::AttackType::FROM_FILE);
    if (app->getBadMsgAttacker().start(filePaths_[index], targetAp_)) {
        EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::BAD_MSG_ACTIVE));
    } else {
        app->showPopUp("Error", "Failed to start attack from file.", nullptr, "OK", "", true);
    }
}

void StationFileListDataSource::drawItem(App* app, U8G2& display, ListMenu* menu, int index, int x, int y, int w, int h, bool isSelected) {
    if (index >= fileNames_.size()) return;

    const std::string& label = fileNames_[index];
    display.setDrawColor(isSelected ? 0 : 1);
    
    char bssidStr[18];
    sprintf(bssidStr, "%02X%02X%02X%02X%02X%02X", targetAp_.bssid[0], targetAp_.bssid[1], targetAp_.bssid[2], targetAp_.bssid[3], targetAp_.bssid[4], targetAp_.bssid[5]);
    
    IconType icon = IconType::INFO;
    // Highlight the file that corresponds to the target BSSID
    if (label.find(bssidStr) != std::string::npos) {
        icon = IconType::TARGET;
    }

    drawCustomIcon(display, x + 4, y + (h - IconSize::LARGE_HEIGHT) / 2, icon);

    int text_x = x + 4 + IconSize::LARGE_WIDTH + 4;
    int text_y = y + h / 2 + 4;
    int text_w = w - (text_x - x) - 4;
    
    menu->updateAndDrawText(display, label.c_str(), text_x, text_y, text_w, isSelected);
}