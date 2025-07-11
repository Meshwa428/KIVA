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
    void onEnter(App* app) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(App* app, InputEvent event) override;

    const char* getTitle() const override { return "Wi-Fi Setup"; }
    MenuType getMenuType() const override { return MenuType::WIFI_LIST; }
    
    void setScanOnEnter(bool scan);

private:
    void scroll(int direction);
    void rebuildDisplayItems(App* app);
    void drawWifiSignal(U8G2& display, int x, int y_center, int8_t rssi);

    enum class ListItemType { NETWORK, SCAN, BACK };
    
    // --- MODIFIED: Self-contained struct ---
    struct DisplayItem {
        std::string label;
        ListItemType type;
        // Data for drawing, copied from source. No more lookups!
        int8_t rssi;
        bool isSecure;
        // We still need the original index for the 'connect' action.
        int originalNetworkIndex; 
    };

    std::vector<DisplayItem> displayItems_;
    int selectedIndex_;
    VerticalListAnimation animation_;
    bool scanOnEnter_ = true;
    uint32_t lastKnownScanCount_;
    bool isScanning_;

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