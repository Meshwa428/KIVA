#include "FirmwareListDataSource.h"
#include "App.h"
#include "OtaManager.h"
#include "UI_Utils.h"
#include "ListMenu.h" // For getting marquee text

FirmwareListDataSource::FirmwareListDataSource() {}

void FirmwareListDataSource::onEnter(App* app, ListMenu* menu, bool isForwardNav) {
    rebuildDisplayItems(app);
}

void FirmwareListDataSource::onExit(App* app, ListMenu* menu) {
    displayItems_.clear();
}

void FirmwareListDataSource::rebuildDisplayItems(App* app) {
    OtaManager& ota = app->getOtaManager();
    ota.scanSdForFirmware();
    
    displayItems_.clear();
    const auto& firmwares = ota.getAvailableFirmwares();

    for(size_t i = 0; i < firmwares.size(); ++i) {
        displayItems_.push_back({firmwares[i].version, false, (int)i});
    }

    if (firmwares.empty()) {
        displayItems_.push_back({"No firmware found", true, -1});
    }
    displayItems_.push_back({"Back", true, -1});
}

int FirmwareListDataSource::getNumberOfItems(App* app) {
    return displayItems_.size();
}

void FirmwareListDataSource::onItemSelected(App* app, ListMenu* menu, int index) {
    const auto& selected = displayItems_[index];
    if (selected.isBackButton) {
        if (selected.label == "Back") {
            app->changeMenu(MenuType::BACK);
        }
        // "No firmware" item is also a back button, but does nothing on click.
    } else {
        const auto& firmwares = app->getOtaManager().getAvailableFirmwares();
        if(selected.firmwareIndex >= 0 && (size_t)selected.firmwareIndex < firmwares.size()) {
            app->getOtaManager().startSdUpdate(firmwares[selected.firmwareIndex]);
            app->changeMenu(MenuType::OTA_STATUS);
        }
    }
}

void FirmwareListDataSource::drawItem(App* app, U8G2& display, ListMenu* menu, int index, int x, int y, int w, int h, bool isSelected) {
    const auto& item = displayItems_[index];

    display.setDrawColor(isSelected ? 0 : 1);

    int icon_x = x + 4;
    int icon_y = y + (h - IconSize::LARGE_HEIGHT) / 2;
    IconType icon = item.isBackButton ? IconType::NAV_BACK : IconType::SETTING_SYSTEM;
    drawCustomIcon(display, icon_x, icon_y, icon, IconRenderSize::LARGE);
    
    int text_x = icon_x + IconSize::LARGE_WIDTH + 4;
    int text_y_base = y + h / 2 + 4;
    int right_padding = 4;
    int text_available_width = (x + w) - text_x - right_padding;

    const char* textToDisplay = item.label.c_str();

    // --- FIX: Use the helper function from ListMenu for text drawing ---
    menu->updateAndDrawText(display, textToDisplay, text_x, text_y_base, text_available_width, isSelected);
}