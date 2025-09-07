#include "BeaconFileListDataSource.h"
#include "App.h"
#include "SdCardManager.h"
#include "ListMenu.h"
#include "UI_Utils.h"
#include "BeaconSpammer.h"
#include "Event.h"
#include "EventDispatcher.h"

void BeaconFileListDataSource::onEnter(App* app, ListMenu* menu, bool isForwardNav) {
    fileNames_.clear();
    const char* dirPath = SD_ROOT::USER_BEACON_LISTS;
    
    // The directory is created at boot by SdCardManager::ensureStandardDirs()
    File root = SdCardManager::openFile(dirPath);
    if (!root || !root.isDirectory()) return;

    File file = root.openNextFile();
    while(file) {
        if (!file.isDirectory() && String(file.name()).endsWith(".txt")) {
            fileNames_.push_back(file.name());
        }
        file.close();
        file = root.openNextFile();
    }
    root.close();

    fileNames_.push_back("Back");
}

void BeaconFileListDataSource::onExit(App* app, ListMenu* menu) {
}

int BeaconFileListDataSource::getNumberOfItems(App* app) {
    return fileNames_.size();
}

void BeaconFileListDataSource::onItemSelected(App* app, ListMenu* menu, int index) {
    if (index >= fileNames_.size()) return;

    std::string selectedFile = fileNames_[index];
    if (selectedFile == "Back") {
        EventDispatcher::getInstance().publish(Event{EventType::NAVIGATE_BACK});
        return;
    }

    std::string fullPath = std::string(SD_ROOT::USER_BEACON_LISTS) + "/" + selectedFile;

    if (!BeaconSpammer::isSsidFileValid(fullPath)) {
        app->showPopUp("Error", "File is empty or invalid.", nullptr, "OK", "", true);
        return; 
    }
    
    BeaconSpamActiveMenu* activeMenu = static_cast<BeaconSpamActiveMenu*>(app->getMenu(MenuType::BEACON_SPAM_ACTIVE));
    if (activeMenu) {
        activeMenu->setAttackParameters(BeaconSsidMode::FILE_BASED, fullPath);
        EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::BEACON_SPAM_ACTIVE));
    }
}

void BeaconFileListDataSource::drawItem(App* app, U8G2& display, ListMenu* menu, int index, int x, int y, int w, int h, bool isSelected) {
    if (index >= fileNames_.size()) return;

    const std::string& label = fileNames_[index];
    display.setDrawColor(isSelected ? 0 : 1);

    IconType icon = (label == "Back") ? IconType::NAV_BACK : IconType::INFO;
    drawCustomIcon(display, x + 4, y + (h - IconSize::LARGE_HEIGHT) / 2, icon);

    int text_x = x + 4 + IconSize::LARGE_WIDTH + 4;
    int text_y = y + h / 2 + 4;
    int text_w = w - (text_x - x) - 4;
    
    menu->updateAndDrawText(display, label.c_str(), text_x, text_y, text_w, isSelected);
}