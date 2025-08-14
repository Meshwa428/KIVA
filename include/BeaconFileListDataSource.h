#ifndef BEACON_FILE_LIST_DATA_SOURCE_H
#define BEACON_FILE_LIST_DATA_SOURCE_H

#include "IListMenuDataSource.h"
#include <vector>
#include <string>

class BeaconFileListDataSource : public IListMenuDataSource {
public:
    int getNumberOfItems(App* app) override;
    void drawItem(App* app, U8G2& display, ListMenu* menu, int index, int x, int y, int w, int h, bool isSelected) override;
    void onItemSelected(App* app, ListMenu* menu, int index) override;
    void onEnter(App* app, ListMenu* menu, bool isForwardNav) override;
    void onExit(App* app, ListMenu* menu) override;

private:
    std::vector<std::string> fileNames_;
};

#endif // BEACON_FILE_LIST_DATA_SOURCE_H