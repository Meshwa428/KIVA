#ifndef WIFI_ATTACKS_DATA_SOURCE_H
#define WIFI_ATTACKS_DATA_SOURCE_H

#include "IListMenuDataSource.h"
#include "IMenu.h" // For MenuItem
#include <vector>

class WifiAttacksDataSource : public IListMenuDataSource {
public:
    WifiAttacksDataSource();
    int getNumberOfItems(App* app) override;
    void drawItem(App* app, U8G2& display, ListMenu* menu, int index, int x, int y, int w, int h, bool isSelected) override;
    void onItemSelected(App* app, ListMenu* menu, int index) override;
    void onEnter(App* app, ListMenu* menu, bool isForwardNav) override {}
    void onExit(App* app, ListMenu* menu) override {}
private:
    std::vector<MenuItem> items_;
};

#endif // WIFI_ATTACKS_DATA_SOURCE_H