#include "Event.h"
#include "EventDispatcher.h"
#include "ChannelSelectionMenu.h"
#include "App.h"
#include "UI_Utils.h"
#include "Jammer.h" // To get JammingMode and JammerConfig

ChannelSelectionMenu::ChannelSelectionMenu() :
    selectedIndex_(0),
    columns_(7), // Using 7 columns for better spacing
    targetScrollOffset_Y_(0.0f),
    currentScrollOffset_Y_(0.0f)
{
    // Default all channels to deselected
    for (int i = 0; i < NUM_NRF_CHANNELS; ++i) {
        channelIsSelected_[i] = false;
    }
}

void ChannelSelectionMenu::onEnter(App* app, bool isForwardNav) {
    EventDispatcher::getInstance().subscribe(EventType::INPUT, this);
    if (isForwardNav) {
        selectedIndex_ = 0;
        targetScrollOffset_Y_ = 0.0f;
        currentScrollOffset_Y_ = 0.0f;
        // Selections are intentionally preserved
    }
}

void ChannelSelectionMenu::onUpdate(App* app) {
    float scrollDiff = targetScrollOffset_Y_ - currentScrollOffset_Y_;
    if (abs(scrollDiff) > 0.1f) {
        currentScrollOffset_Y_ += scrollDiff * GRID_ANIM_SPEED * 0.016f;
    } else {
        currentScrollOffset_Y_ = targetScrollOffset_Y_;
    }
}

void ChannelSelectionMenu::onExit(App* app) {
    EventDispatcher::getInstance().unsubscribe(EventType::INPUT, this);
    // No specific cleanup needed
}

void ChannelSelectionMenu::handleInput(InputEvent event, App* app) {
    switch(event) {
        case InputEvent::BTN_UP_PRESS:
            scroll(-columns_);
            break;
        case InputEvent::BTN_DOWN_PRESS:
            scroll(columns_);
            break;
        case InputEvent::BTN_LEFT_PRESS:
        case InputEvent::ENCODER_CCW:
            scroll(-1);
            break;
        case InputEvent::BTN_RIGHT_PRESS:
        case InputEvent::ENCODER_CW:
            scroll(1);
            break;
        case InputEvent::BTN_OK_PRESS:
        case InputEvent::BTN_ENCODER_PRESS:
            if (selectedIndex_ < NUM_NRF_CHANNELS) {
                toggleChannel(selectedIndex_);
            } else if (selectedIndex_ == BUTTON_START_IDX) {
                startJamming(app);
            } else if (selectedIndex_ == BUTTON_ALL_IDX) {
                for(int i=0; i < NUM_NRF_CHANNELS; ++i) channelIsSelected_[i] = true;
            } else if (selectedIndex_ == BUTTON_NONE_IDX) {
                for(int i=0; i < NUM_NRF_CHANNELS; ++i) channelIsSelected_[i] = false;
            }
            break;
        case InputEvent::BTN_BACK_PRESS:
            EventDispatcher::getInstance().publish(Event{EventType::NAVIGATE_BACK});
            break;
        default:
            break;
    }
}

void ChannelSelectionMenu::scroll(int direction) {
    int oldSelectedIndex = selectedIndex_;
    selectedIndex_ += direction;

    // Clamp the index within the valid range
    if (selectedIndex_ < 0) selectedIndex_ = 0;
    if (selectedIndex_ >= TOTAL_SELECTABLE_ITEMS) selectedIndex_ = TOTAL_SELECTABLE_ITEMS - 1;

    bool wasOnButton = (oldSelectedIndex >= NUM_NRF_CHANNELS);
    bool isOnButton = (selectedIndex_ >= NUM_NRF_CHANNELS);

    // --- Common layout constants for scrolling logic ---
    const int itemH = 12;
    const int itemPadding = 2;
    const int itemRowHeight = itemH + itemPadding;
    const int buttonH = 12;
    const int gridStartY = STATUS_BAR_H + 2;
    const int gridVisibleAreaY_end = 63;
    const int gridVisibleAreaH = 64 - (STATUS_BAR_H + 1);
    const int visibleRows = gridVisibleAreaH / itemRowHeight;

    if (isOnButton) {
        // --- Case 1: Scrolling TO or BETWEEN buttons ---
        // Calculate the maximum scroll offset required to show the buttons at the very bottom.
        int num_grid_rows = (NUM_NRF_CHANNELS + columns_ - 1) / columns_;
        int buttonsTopY_unscrolled = gridStartY + num_grid_rows * itemRowHeight + 4;
        int buttonsBottomY_unscrolled = buttonsTopY_unscrolled + buttonH;
        
        // --- MODIFIED LINE ---
        // Calculate the offset to bring buttons into view, and add 2px for a bottom margin.
        float maxScrollOffset = buttonsBottomY_unscrolled - gridVisibleAreaY_end + 2;
        if (maxScrollOffset < 0) maxScrollOffset = 0; // Don't scroll if everything fits.
        
        targetScrollOffset_Y_ = maxScrollOffset;
        
    } else {
        // --- Case 2: Scrolling within the grid ---
        int currentSelectionRow = selectedIndex_ / columns_;
        
        if (wasOnButton && !isOnButton) {
            // Special transition: moving from a button UP to the last grid row.
            // We must force a scroll, as the item is already "visible" but too low.
            // Position it as the last visible row.
            targetScrollOffset_Y_ = (currentSelectionRow - visibleRows + 1) * itemRowHeight;
        } else {
            // Standard grid-to-grid scrolling.
            int topVisibleRow = (int)(targetScrollOffset_Y_ / itemRowHeight);
            
            if (currentSelectionRow < topVisibleRow) {
                // Scroll up because the item is above the view
                targetScrollOffset_Y_ = currentSelectionRow * itemRowHeight;
            } else if (currentSelectionRow >= topVisibleRow + visibleRows) {
                // Scroll down because the item is below the view
                targetScrollOffset_Y_ = (currentSelectionRow - visibleRows + 1) * itemRowHeight;
            }
        }
    }
}

void ChannelSelectionMenu::toggleChannel(int index) {
    if (index >= 0 && index < NUM_NRF_CHANNELS) {
        channelIsSelected_[index] = !channelIsSelected_[index];
    }
}

void ChannelSelectionMenu::startJamming(App* app) {
    JammerConfig config;
    for (int i = 0; i < NUM_NRF_CHANNELS; ++i) {
        if (channelIsSelected_[i]) {
            config.customChannels.push_back(i);
        }
    }

    if (config.customChannels.empty()) {
        app->showPopUp("Error", "No channels were selected.", nullptr, "OK", "", true);
        return;
    }
    
    // --- REVERTED FOR FAST SWEEP ---
    config.technique = JammingTechnique::NOISE_INJECTION;

    // 1. Get the destination menu instance.
    JammingActiveMenu* jammerMenu = static_cast<JammingActiveMenu*>(app->getMenu(MenuType::JAMMING_ACTIVE));
    if (jammerMenu) {
        // 2. Configure it with the mode and custom channels.
        jammerMenu->setJammingModeToStart(JammingMode::CHANNEL_FLOOD_CUSTOM);
        jammerMenu->setJammingConfig(config);
        
        // 3. Navigate to it. Its onEnter() will do the work.
        EventDispatcher::getInstance().publish(NavigateToMenuEvent(MenuType::JAMMING_ACTIVE));
    } else {
        // This should not happen if the menu is registered correctly.
        app->showPopUp("Fatal Error", "Jamming menu not found.", nullptr, "OK", "", true);
    }
}

void ChannelSelectionMenu::draw(App* app, U8G2& display) {
    const int gridStartY = STATUS_BAR_H + 2;
    // --- Grid layout values ---
    const int itemW = 16;
    const int itemH = 12;
    const int itemPadding = 2;
    const int gridMarginX = 2;
    
    // Use a font that can clearly display 3-digit numbers
    display.setFont(u8g2_font_5x7_tf);
    
    display.setClipWindow(0, STATUS_BAR_H + 1, 127, 63);

    // Draw Channel Grid
    for (int i = 0; i < NUM_NRF_CHANNELS; ++i) {
        int row = i / columns_;
        int col = i % columns_;
        
        int boxX = gridMarginX + col * (itemW + itemPadding);
        int boxY = gridStartY + row * (itemH + itemPadding) - (int)currentScrollOffset_Y_;

        if (boxY + itemH < gridStartY || boxY > 63) continue;

        bool isSelected = (i == selectedIndex_);
        
        // Draw selection highlight first
        if (isSelected) {
            display.setDrawColor(1);
            display.drawFrame(boxX - 1, boxY - 1, itemW + 2, itemH + 2);
        }
        
        // Draw box
        display.setDrawColor(1);
        if (channelIsSelected_[i]) {
            display.drawBox(boxX, boxY, itemW, itemH);
            display.setDrawColor(0); // Text color for selected items
        } else {
            display.drawFrame(boxX, boxY, itemW, itemH);
        }

        // Draw text
        char numStr[4];
        itoa(i, numStr, 10);
        // Center text in the larger box
        display.drawStr(boxX + (itemW - display.getStrWidth(numStr)) / 2, boxY + 9, numStr);

        display.setDrawColor(1); // Reset draw color
    }
    
    // Draw Buttons at the bottom
    int num_grid_rows = (NUM_NRF_CHANNELS + columns_ - 1) / columns_;
    int buttonY = gridStartY + num_grid_rows * (itemH + itemPadding) + 4 - (int)currentScrollOffset_Y_;

    // --- Button layout values ---
    display.setFont(u8g2_font_5x7_tf);
    const char* startLabel = "START";
    const char* allLabel = "ALL";
    const char* noneLabel = "NONE";
    const int buttonTextPadding = 8; // 4px on each side
    const int buttonH = 12;
    const int buttonSpacing = 4;

    int startW = display.getStrWidth(startLabel) + buttonTextPadding;
    int allW = display.getStrWidth(allLabel) + buttonTextPadding;
    int noneW = display.getStrWidth(noneLabel) + buttonTextPadding;
    
    int totalButtonWidth = startW + allW + noneW + 2 * buttonSpacing;
    int buttonStartX = (128 - totalButtonWidth) / 2;

    int startX = buttonStartX;
    int allX = startX + startW + buttonSpacing;
    int noneX = allX + allW + buttonSpacing;
    
    // Start Button
    drawRndBox(display, startX, buttonY, startW, buttonH, 1, selectedIndex_ == BUTTON_START_IDX);
    display.setDrawColor(selectedIndex_ == BUTTON_START_IDX ? 0 : 1);
    display.drawStr(startX + buttonTextPadding / 2, buttonY + 9, startLabel);
    
    // All Button
    display.setDrawColor(1);
    drawRndBox(display, allX, buttonY, allW, buttonH, 1, selectedIndex_ == BUTTON_ALL_IDX);
    display.setDrawColor(selectedIndex_ == BUTTON_ALL_IDX ? 0 : 1);
    display.drawStr(allX + buttonTextPadding / 2, buttonY + 9, allLabel);
    
    // None Button
    display.setDrawColor(1);
    drawRndBox(display, noneX, buttonY, noneW, buttonH, 1, selectedIndex_ == BUTTON_NONE_IDX);
    display.setDrawColor(selectedIndex_ == BUTTON_NONE_IDX ? 0 : 1);
    display.drawStr(noneX + buttonTextPadding / 2, buttonY + 9, noneLabel);

    display.setDrawColor(1);
    display.setMaxClipWindow();
}
