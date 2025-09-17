#ifndef CHANNEL_SELECTION_MENU_H
#define CHANNEL_SELECTION_MENU_H

#include "IMenu.h"
#include "Config.h" // For MenuType
#include <vector>

#define NUM_NRF_CHANNELS 126
#define BUTTON_START_IDX NUM_NRF_CHANNELS
#define BUTTON_ALL_IDX (NUM_NRF_CHANNELS + 1)
#define BUTTON_NONE_IDX (NUM_NRF_CHANNELS + 2)
#define TOTAL_SELECTABLE_ITEMS (NUM_NRF_CHANNELS + 3)

class ChannelSelectionMenu : public IMenu {
public:
    ChannelSelectionMenu();

    void onEnter(App* app, bool isForwardNav) override;
    void onUpdate(App* app) override;
    void onExit(App* app) override;
    void draw(App* app, U8G2& display) override;
    void handleInput(InputEvent event, App* app) override;

    const char* getTitle() const override { return "Select Channels"; }
    MenuType getMenuType() const override { return MenuType::CHANNEL_SELECTION; }

private:
    void scroll(int direction);
    void toggleChannel(int index);
    void startJamming(App* app);
    
    // UI state
    int selectedIndex_;
    int columns_;
    float targetScrollOffset_Y_;
    float currentScrollOffset_Y_;

    // Channel state
    bool channelIsSelected_[NUM_NRF_CHANNELS];
};

#endif // CHANNEL_SELECTION_MENU_H