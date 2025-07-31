#include "DuckyScriptListDataSource.h"
#include "App.h"
#include "SdCardManager.h"
#include "ListMenu.h"
#include "UI_Utils.h"
#include "BadUSB.h"

void DuckyScriptListDataSource::onEnter(App* app, ListMenu* menu, bool isForwardNav) {
    fileNames_.clear();
    filePaths_.clear();
    const char* dirPath = SD_ROOT::USER_DUCKY;
    
    File root = SdCardManager::openFile(dirPath);
    if (!root || !root.isDirectory()) return;

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

void DuckyScriptListDataSource::onExit(App* app, ListMenu* menu) {}

int DuckyScriptListDataSource::getNumberOfItems(App* app) {
    return fileNames_.size();
}

const std::string& DuckyScriptListDataSource::getSelectedScriptPath(int index) {
    return filePaths_[index];
}

void DuckyScriptListDataSource::onItemSelected(App* app, ListMenu* menu, int index) {
    if (index >= filePaths_.size()) return;

    // We don't start the script directly. Instead, we configure and switch
    // to the mode selection menu, passing the chosen script path.
    app->getBadUsbModeSelectionMenu().setScriptPath(filePaths_[index]);
    app->changeMenu(MenuType::BAD_USB_MODE_SELECTION);
}

void DuckyScriptListDataSource::drawItem(App* app, U8G2& display, ListMenu* menu, int index, int x, int y, int w, int h, bool isSelected) {
    if (index >= fileNames_.size()) return;

    const std::string& label = fileNames_[index];
    display.setDrawColor(isSelected ? 0 : 1);

    drawCustomIcon(display, x + 4, y + (h - IconSize::LARGE_HEIGHT) / 2, IconType::TOOL_INJECTION);

    int text_x = x + 4 + IconSize::LARGE_WIDTH + 4;
    int text_y = y + h / 2 + 4;
    int text_w = w - (text_x - x) - 4;
    
    menu->updateAndDrawText(display, label.c_str(), text_x, text_y, text_w, isSelected);
}