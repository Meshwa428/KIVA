#ifndef DUCKY_SCRIPT_LIST_DATA_SOURCE_H
#define DUCKY_SCRIPT_LIST_DATA_SOURCE_H

#include "IListMenuDataSource.h"
#include "DuckyScriptRunner.h" // Include the new header
#include <vector>
#include <string>

class DuckyScriptListDataSource : public IListMenuDataSource {
public:
    DuckyScriptListDataSource(); // Add constructor
    
    int getNumberOfItems(App* app) override;
    void drawItem(App* app, U8G2& display, ListMenu* menu, int index, int x, int y, int w, int h, bool isSelected) override;
    void onItemSelected(App* app, ListMenu* menu, int index) override;
    void onEnter(App* app, ListMenu* menu, bool isForwardNav) override;
    void onExit(App* app, ListMenu* menu) override;
    
    const std::string& getSelectedScriptPath(int index);

    // --- NEW METHOD ---
    void setExecutionMode(DuckyScriptRunner::Mode mode);

private:
    std::vector<std::string> fileNames_;
    std::vector<std::string> filePaths_;
    DuckyScriptRunner::Mode modeToExecute_; // --- NEW MEMBER ---
};

#endif // DUCKY_SCRIPT_LIST_DATA_SOURCE_H