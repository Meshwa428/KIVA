#ifndef WIFI_LIST_DATA_SOURCE_H
#define WIFI_LIST_DATA_SOURCE_H

#include "IListMenuDataSource.h"
#include "WifiManager.h"
#include <vector>
#include <string>

class WifiListDataSource : public IListMenuDataSource {
public:
    WifiListDataSource();

    // IListMenuDataSource implementation
    int getNumberOfItems(App* app) override;
    void drawItem(App* app, U8G2& display, ListMenu* menu, int index, int x, int y, int w, int h, bool isSelected) override;
    void onItemSelected(App* app, ListMenu* menu, int index) override;
    void onEnter(App* app, ListMenu* menu, bool isForwardNav) override;
    void onExit(App* app, ListMenu* menu) override;
    void onUpdate(App* app, ListMenu* menu) override;
    bool drawCustomEmptyMessage(App* app, U8G2& display) override;

    // Specific methods for this data source
    void setScanOnEnter(bool scan);
    void setBackNavOverride(bool override);
    void forceReload(App* app, ListMenu* menu);

private:
    void rebuildDisplayItems(App* app);
    void drawWifiSignal(U8G2& display, int x, int y_center, int8_t rssi);

    enum class ListItemType { NETWORK, SCAN, BACK };
    
    struct DisplayItem {
        std::string label;
        ListItemType type;
        int8_t rssi;
        bool isSecure;
        int originalNetworkIndex; 
    };

    std::vector<DisplayItem> displayItems_;
    bool scanOnEnter_ = true;
    uint32_t lastKnownScanCount_ = 0;
    bool isScanning_ = false;
    bool backNavOverride_ = false;
};

#endif // WIFI_LIST_DATA_SOURCE_H