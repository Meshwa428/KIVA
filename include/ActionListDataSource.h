#ifndef ACTION_LIST_DATA_SOURCE_H
#define ACTION_LIST_DATA_SOURCE_H

#include "IListMenuDataSource.h"
#include "IMenu.h" // For MenuItem
#include <vector>

class ActionListDataSource : public IListMenuDataSource {
public:
    ActionListDataSource(const std::vector<MenuItem>& items);

    int getNumberOfItems(App* app) override;
    void drawItem(App* app, U8G2& display, ListMenu* menu, int index, int x, int y, int w, int h, bool isSelected) override;
    void onItemSelected(App* app, ListMenu* menu, int index) override;
    
    // --- NEW: Implement the getItem method ---
    const MenuItem* getItem(int index) const override;
    
    // Default implementations for non-dynamic lists
    void onEnter(App* app, ListMenu* menu, bool isForwardNav) override {}
    void onExit(App* app, ListMenu* menu) override {}
private:
    std::vector<MenuItem> items_;
};

#endif // ACTION_LIST_DATA_SOURCE_H