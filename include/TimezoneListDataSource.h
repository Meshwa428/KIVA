#ifndef TIMEZONE_LIST_DATA_SOURCE_H
#define TIMEZONE_LIST_DATA_SOURCE_H

#include "IListMenuDataSource.h"
#include <vector>
#include <string>

struct TimezoneInfo {
    const char* displayName;      // e.g., "US Eastern (EST/EDT)"
    const char* shortName;        // e.g., "US Eastern"
    const char* posixTzString;    // e.g., "EST5EDT,M3.2.0,M11.1.0"
};

class TimezoneListDataSource : public IListMenuDataSource {
public:
    TimezoneListDataSource();

    int getNumberOfItems(App* app) override;
    void drawItem(App* app, U8G2& display, ListMenu* menu, int index, int x, int y, int w, int h, bool isSelected) override;
    void onItemSelected(App* app, ListMenu* menu, int index) override;
    
    // These can be empty for a static list
    void onEnter(App* app, ListMenu* menu, bool isForwardNav) override;
    void onExit(App* app, ListMenu* menu) override;

    // a safe, lazy-loading data provider
    const std::vector<TimezoneInfo>& getTimezones();

private:
    std::vector<TimezoneInfo> timezones_;
};

#endif // TIMEZONE_LIST_DATA_SOURCE_H