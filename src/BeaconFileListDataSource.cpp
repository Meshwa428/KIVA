#include "BeaconFileListDataSource.h"
#include "App.h"
#include "SdCardManager.h"
#include "ListMenu.h"
#include "UI_Utils.h"

void BeaconFileListDataSource::onEnter(App* app, ListMenu* menu) {
    fileNames_.clear();
    const char* dirPath = "/wifi/beacon_attack";
    if (!SdCardManager::isAvailable() || !SdCardManager::exists(dirPath)) {
        SdCardManager::createDir(dirPath); // Create dir if it doesn't exist
    }

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

    // Always add a "Back" option
    fileNames_.push_back("Back");
}

void BeaconFileListDataSource::onExit(App* app, ListMenu* menu) {
    fileNames_.clear();
}

int BeaconFileListDataSource::getNumberOfItems(App* app) {
    return fileNames_.size();
}

void BeaconFileListDataSource::onItemSelected(App* app, ListMenu* menu, int index) {
    if (index >= fileNames_.size()) return;

    std::string selectedFile = fileNames_[index];
    if (selectedFile == "Back") {
        app->changeMenu(MenuType::BACK);
        return;
    }

    std::string fullPath = "/wifi/beacon_attack/" + selectedFile;
    
    BeaconSpamActiveMenu* activeMenu = static_cast<BeaconSpamActiveMenu*>(app->getMenu(MenuType::BEACON_SPAM_ACTIVE));
    if (activeMenu) {
        activeMenu->setAttackParameters(BeaconSsidMode::FILE_BASED, fullPath);
        app->changeMenu(MenuType::BEACON_SPAM_ACTIVE);
    }
}

void BeaconFileListDataSource::drawItem(App* app, U8G2& display, ListMenu* menu, int index, int x, int y, int w, int h, bool isSelected) {
    const std::string& label = fileNames_[index];
    display.setDrawColor(isSelected ? 0 : 1);

    IconType icon = (label == "Back") ? IconType::NAV_BACK : IconType::INFO;
    drawCustomIcon(display, x + 4, y + (h - IconSize::LARGE_HEIGHT) / 2, icon);

    int text_x = x + 4 + IconSize::LARGE_WIDTH + 4;
    int text_y = y + h / 2 + 4;
    int text_w = w - (text_x - x) - 4;
    
    menu->updateAndDrawText(display, label.c_str(), text_x, text_y, text_w, isSelected);
}