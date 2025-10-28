#ifndef STATION_FILE_LIST_DATA_SOURCE_H
#define STATION_FILE_LIST_DATA_SOURCE_H

#include "IListMenuDataSource.h"
#include "WifiManager.h" // For WifiNetworkInfo
#include <vector>
#include <string>

class StationFileListDataSource : public IListMenuDataSource {
public:
    void setTargetAp(const WifiNetworkInfo& target);

    int getNumberOfItems(App* app) override;
    void drawItem(App* app, U8G2& display, ListMenu* menu, int index, int x, int y, int w, int h, bool isSelected) override;
    void onItemSelected(App* app, ListMenu* menu, int index) override;
    void onEnter(App* app, ListMenu* menu, bool isForwardNav) override;
    void onExit(App* app, ListMenu* menu) override;

private:
    std::vector<std::string> fileNames_;
    std::vector<std::string> filePaths_;
    WifiNetworkInfo targetAp_;
};

#endif // STATION_FILE_LIST_DATA_SOURCE_H