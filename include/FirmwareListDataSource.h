#ifndef FIRMWARE_LIST_DATA_SOURCE_H
#define FIRMWARE_LIST_DATA_SOURCE_H

#include "IListMenuDataSource.h"
#include "Firmware.h"
#include <vector>
#include <string>

class FirmwareListDataSource : public IListMenuDataSource {
public:
    FirmwareListDataSource();

    // IListMenuDataSource implementation
    int getNumberOfItems(App* app) override;
    void drawItem(App* app, U8G2& display, ListMenu* menu, int index, int x, int y, int w, int h, bool isSelected) override;
    void onItemSelected(App* app, ListMenu* menu, int index) override;
    void onEnter(App* app, ListMenu* menu, bool isForwardNav) override;
    void onExit(App* app, ListMenu* menu) override;

private:
    void rebuildDisplayItems(App* app);
    
    struct DisplayItem {
        std::string label;
        bool isBackButton;
        int firmwareIndex; // Index into OtaManager's list
    };
    
    std::vector<DisplayItem> displayItems_;
};

#endif // FIRMWARE_LIST_DATA_SOURCE_H