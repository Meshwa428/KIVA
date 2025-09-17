#ifndef WIFI_LIST_MENU_H
#define WIFI_LIST_MENU_H

#include "IMenu.h"
#include "Animation.h"
#include "WifiManager.h" 
#include <vector>
#include <string>

class WifiListMenu : public IMenu {
public:
    WifiListMenu();
    void onEnter(App* app, bool isForwardNav) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(InputEvent event, App* app) override;

    const char* getTitle() const override { return "Wi-Fi Setup"; }
    MenuType getMenuType() const override { return MenuType::WIFI_LIST; }
    
    void setScanOnEnter(bool scan);
    void setBackNavOverride(bool override); // <-- NEW

private:
    void scroll(int direction);
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
    int selectedIndex_;
    VerticalListAnimation animation_;
    bool scanOnEnter_ = true;
    uint32_t lastKnownScanCount_;
    bool isScanning_;
    bool backNavOverride_ = false; // <-- NEW FLAG

    // Marquee State
    char marqueeText_[40];
    int marqueeTextLenPx_;
    float marqueeOffset_;
    unsigned long lastMarqueeTime_;
    bool marqueeActive_;
    bool marqueePaused_;
    unsigned long marqueePauseStartTime_;
    bool marqueeScrollLeft_;
};

#endif // WIFI_LIST_MENU_H