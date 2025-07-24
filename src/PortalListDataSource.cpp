#include "PortalListDataSource.h"
#include "App.h"
#include "SdCardManager.h"
#include "ListMenu.h"
#include "UI_Utils.h"
#include "EvilTwin.h"

void PortalListDataSource::onEnter(App* app, ListMenu* menu) {
    portalNames_.clear();
    const char* dirPath = SD_ROOT::USER_PORTALS;
    if (!SdCardManager::isAvailable()) {
        app->showPopUp("Error", "SD Card not found.", nullptr, "OK", "", true);
        return;
    }
    
    File root = SdCardManager::openFile(dirPath);
    if (!root || !root.isDirectory()) return;

    File file = root.openNextFile();
    while(file) {
        if (file.isDirectory()) {
            portalNames_.push_back(file.name());
        }
        file.close();
        file = root.openNextFile();
    }
    root.close();

    portalNames_.push_back("Back");
}

void PortalListDataSource::onExit(App* app, ListMenu* menu) {
}

int PortalListDataSource::getNumberOfItems(App* app) {
    return portalNames_.size();
}

void PortalListDataSource::onItemSelected(App* app, ListMenu* menu, int index) {
    if (index >= portalNames_.size()) return;

    std::string selectedPortal = portalNames_[index];
    if (selectedPortal == "Back") {
        app->changeMenu(MenuType::BACK);
        return;
    }

    app->getEvilTwin().prepareAttack();
    app->getEvilTwin().setSelectedPortal(selectedPortal);
    
    app->getWifiListDataSource().setScanOnEnter(true);
    app->getWifiListDataSource().setBackNavOverride(false);
    app->changeMenu(MenuType::WIFI_LIST);
}

void PortalListDataSource::drawItem(App* app, U8G2& display, ListMenu* menu, int index, int x, int y, int w, int h, bool isSelected) {
    if (index >= portalNames_.size()) return;

    const std::string& label = portalNames_[index];
    display.setDrawColor(isSelected ? 0 : 1);

    IconType icon = (label == "Back") ? IconType::NAV_BACK : IconType::INFO;
    drawCustomIcon(display, x + 4, y + (h - IconSize::LARGE_HEIGHT) / 2, icon);

    int text_x = x + 4 + IconSize::LARGE_WIDTH + 4;
    int text_y = y + h / 2 + 4;
    int text_w = w - (text_x - x) - 4;
    
    menu->updateAndDrawText(display, label.c_str(), text_x, text_y, text_w, isSelected);
}