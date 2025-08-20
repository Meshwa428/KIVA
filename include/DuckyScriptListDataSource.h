#ifndef DUCKY_SCRIPT_LIST_DATA_SOURCE_H
#define DUCKY_SCRIPT_LIST_DATA_SOURCE_H

#include "IListMenuDataSource.h"
#include <HIDForge.h>
#include <vector>
#include <string>

class DuckyScriptListDataSource : public IListMenuDataSource {
public:
    DuckyScriptListDataSource();
    
    int getNumberOfItems(App* app) override;
    void drawItem(App* app, U8G2& display, ListMenu* menu, int index, int x, int y, int w, int h, bool isSelected) override;
    void onItemSelected(App* app, ListMenu* menu, int index) override;
    void onEnter(App* app, ListMenu* menu, bool isForwardNav) override;
    void onExit(App* app, ListMenu* menu) override;
    
    // This method is updated to track the interface type.
    void setHidInterface(HIDInterface* hid, bool isUsb);

private:
    std::vector<std::string> fileNames_;
    std::vector<std::string> filePaths_;
    HIDInterface* hidInterfaceToUse_;
    bool isUsbInterface_;
};

#endif // DUCKY_SCRIPT_LIST_DATA_SOURCE_H