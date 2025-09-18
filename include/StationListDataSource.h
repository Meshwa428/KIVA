#ifndef STATION_LIST_DATA_SOURCE_H
#define STATION_LIST_DATA_SOURCE_H

#include "IListMenuDataSource.h"
#include "StationSniffer.h"
#include <functional>
#include <vector>
#include <string>

class StationListDataSource : public IListMenuDataSource {
public:
    using AttackCallback = std::function<void(App*, const StationInfo&)>;

    StationListDataSource();

    void setMode(bool liveScan, const WifiNetworkInfo& target, const std::string& filePath = "");
    void setAttackCallback(AttackCallback callback);

    int getNumberOfItems(App* app) override;
    void drawItem(App* app, U8G2& display, ListMenu* menu, int index, int x, int y, int w, int h, bool isSelected) override;
    void onItemSelected(App* app, ListMenu* menu, int index) override;
    void onEnter(App* app, ListMenu* menu, bool isForwardNav) override;
    void onExit(App* app, ListMenu* menu) override;
    void onUpdate(App* app, ListMenu* menu) override;
    bool drawCustomEmptyMessage(App* app, U8G2& display) override;

private:
    void loadStationsFromFile();

    bool isLiveScan_;
    WifiNetworkInfo targetAp_; // Only for live scan
    std::string stationsFilePath_; // Only for file mode
    AttackCallback attackCallback_;
    
    std::vector<StationInfo> stations_;
    size_t lastKnownStationCount_;
};

#endif // STATION_LIST_DATA_SOURCE_H