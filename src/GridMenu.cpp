#include "GridMenu.h"
#include "App.h"
#include "UI_Utils.h"
#include <Arduino.h>
#include <algorithm> // For std::min

GridMenu::GridMenu(std::string title, std::vector<MenuItem> items, int columns) : 
    title_(title),
    menuItems_(items),
    columns_(columns),
    selectedIndex_(0),
    gridAnimatingIn_(false),
    marqueeActive_(false),
    marqueeScrollLeft_(true)
{
    // Infer MenuType from title
    if (title == "WiFi Tools") menuType_ = MenuType::WIFI_TOOLS_GRID;
    else if (title == "Update") menuType_ = MenuType::FIRMWARE_UPDATE_GRID;
    else if (title == "Jammer") menuType_ = MenuType::JAMMING_TOOLS_GRID;
    else if (title == "Deauth Attack") menuType_ = MenuType::DEAUTH_TOOLS_GRID;
    else menuType_ = MenuType::NONE;
}

void GridMenu::onEnter(App *app, bool isForwardNav)
{
    if (isForwardNav) {
        selectedIndex_ = 0;
        targetGridScrollOffset_Y_ = 0;
        currentGridScrollOffset_Y_anim_ = 0;
        marqueeScrollLeft_ = true;
    }
    // Resize animation vectors to match the number of items
    gridItemScale_.resize(menuItems_.size());
    gridItemAnimStartTime_.resize(menuItems_.size());

    startGridAnimation();
    marqueeActive_ = false;
}

void GridMenu::onUpdate(App *app)
{
    float scrollDiff = targetGridScrollOffset_Y_ - currentGridScrollOffset_Y_anim_;
    if (abs(scrollDiff) > 0.1f)
    {
        currentGridScrollOffset_Y_anim_ += scrollDiff * GRID_ANIM_SPEED * 0.016f;
    }
    else
    {
        currentGridScrollOffset_Y_anim_ = targetGridScrollOffset_Y_;
    }

    if (gridAnimatingIn_)
    {
        bool stillAnimating = false;
        unsigned long currentTime = millis();
        for (size_t i = 0; i < menuItems_.size(); ++i)
        {
            if (currentTime >= gridItemAnimStartTime_[i])
            {
                float diffScale = 1.0f - gridItemScale_[i];
                if (abs(diffScale) > 0.01f)
                {
                    gridItemScale_[i] += diffScale * GRID_ANIM_SPEED * 0.016f;
                    stillAnimating = true;
                }
                else
                {
                    gridItemScale_[i] = 1.0f;
                }
            }
            else
            {
                stillAnimating = true;
            }
        }
        if (!stillAnimating)
            gridAnimatingIn_ = false;
    }
}

void GridMenu::onExit(App *app)
{
    marqueeActive_ = false;
}

void GridMenu::handleInput(App* app, InputEvent event) {
    switch(event) {
        case InputEvent::BTN_DOWN_PRESS:
            scroll(columns_); // Move down one row
            break;
        case InputEvent::BTN_UP_PRESS:
            scroll(-columns_); // Move up one row
            break;
        case InputEvent::ENCODER_CW:
        case InputEvent::BTN_RIGHT_PRESS:
            scroll(1); // Move to next column
            break;
        case InputEvent::ENCODER_CCW:
        case InputEvent::BTN_LEFT_PRESS:
            scroll(-1); // Move to previous column
            break;
        case InputEvent::BTN_ENCODER_PRESS:
        case InputEvent::BTN_OK_PRESS:
            {
                if (selectedIndex_ >= menuItems_.size()) break;
                const auto& selected = menuItems_[selectedIndex_];

                if (selected.action) {
                    selected.action(app);
                } else {
                    app->changeMenu(selected.targetMenu);
                }
            }
            break;

        case InputEvent::BTN_BACK_PRESS:
            app->changeMenu(MenuType::BACK);
            break;
        default:
            break;
    }
}

void GridMenu::scroll(int direction)
{
    if (menuItems_.empty()) return;

    int oldIndex = selectedIndex_;
    int newIndex = oldIndex;
    int numItems = menuItems_.size();

    if (abs(direction) == 1) { // --- HORIZONTAL / LINEAR NAVIGATION (Snake) ---
        newIndex = (oldIndex + direction + numItems) % numItems;
    } else { // --- VERTICAL NAVIGATION (Column Wrap) ---
        int currentCol = oldIndex % columns_;
        int currentRow = oldIndex / columns_;

        if (direction > 0) { // NAV_DOWN
            newIndex = oldIndex + columns_;
            if (newIndex >= numItems) { // If we're past the end
                // Wrap to the top of the next column
                int nextCol = (currentCol + 1) % columns_;
                newIndex = nextCol;
                // If the next column is also out of bounds (for a very short last row), wrap to start
                if(newIndex >= numItems) newIndex = 0;
            }
        } else { // NAV_UP
            newIndex = oldIndex - columns_;
            if (newIndex < 0) { // If we're before the start
                // Wrap to the bottom of the previous column
                int prevCol = (currentCol - 1 + columns_) % columns_;
                // Find the last item in that column
                newIndex = prevCol;
                while (newIndex + columns_ < numItems) {
                    newIndex += columns_;
                }
            }
        }
    }

    selectedIndex_ = newIndex;

    // --- Scrolling animation logic ---
    const int itemRowHeight = 28 + 4; // Updated for taller items with icons
    const int gridVisibleAreaH = 64 - (STATUS_BAR_H + 1) - 4;
    const int visibleRows = gridVisibleAreaH / itemRowHeight;

    int currentSelectionRow = selectedIndex_ / columns_;
    int topVisibleRow = (int)(targetGridScrollOffset_Y_ / itemRowHeight);

    if (currentSelectionRow < topVisibleRow) {
        targetGridScrollOffset_Y_ = currentSelectionRow * itemRowHeight;
    } else if (currentSelectionRow >= topVisibleRow + visibleRows) {
        targetGridScrollOffset_Y_ = (currentSelectionRow - visibleRows + 1) * itemRowHeight;
    }
    
    marqueeActive_ = false;
    marqueeScrollLeft_ = true;
}

void GridMenu::startGridAnimation()
{
    gridAnimatingIn_ = true;
    unsigned long currentTime = millis();
    for (size_t i = 0; i < menuItems_.size(); ++i)
    {
        gridItemScale_[i] = 0.0f;
        int row = i / columns_;
        int col = i % columns_;
        gridItemAnimStartTime_[i] = currentTime + (unsigned long)((row * 1.5f + col) * GRID_ANIM_STAGGER_DELAY);
    }
}

void GridMenu::draw(App* app, U8G2& display) {
    const int itemBaseW = (128 - (columns_ + 1) * 4) / columns_;
    const int itemBaseH = 28; // Increased height for icon + text
    const int gridStartY = STATUS_BAR_H + 1 + 4;
    
    const int clip_y_start = STATUS_BAR_H + 1;
    display.setClipWindow(0, clip_y_start, 127, 63);

    display.setFont(u8g2_font_5x7_tf);

    for (size_t i = 0; i < menuItems_.size(); ++i) {
        int row = i / columns_;
        int col = i % columns_;

        float scale = gridAnimatingIn_ ? gridItemScale_[i] : 1.0f;
        int itemW = (int)(itemBaseW * scale);
        int itemH = (int)(itemBaseH * scale);
        if (itemW < 1 || itemH < 1) continue;

        int box_x = 4 + col * (itemBaseW + 4) + (itemBaseW - itemW) / 2;
        int box_y_noscroll = gridStartY + row * (itemBaseH + 4) + (itemBaseH - itemH) / 2;
        int box_y = box_y_noscroll - (int)currentGridScrollOffset_Y_anim_;
        
        if (box_y + itemH <= clip_y_start || box_y >= 64) continue;
        
        bool isSelected = (i == selectedIndex_);
        if (isSelected) {
            display.setDrawColor(1); 
            drawRndBox(display, box_x, box_y, itemW, itemH, 2, true);
            display.setDrawColor(0);
        } else {
            display.setDrawColor(1); 
            drawRndBox(display, box_x, box_y, itemW, itemH, 2, false);
        }
        
        if (scale > 0.7f) {
            // --- NEW: Icon Drawing Logic ---
            int icon_y = box_y + 2;
            int icon_x = box_x + (itemW - IconSize::LARGE_WIDTH) / 2;
            drawCustomIcon(display, icon_x, icon_y, menuItems_[i].icon);

            // --- Text Drawing Logic (Adjusted for position) ---
            const int text_padding = 2;
            int innerW = itemW - (2 * text_padding);
            int text_y = box_y + itemH - 3; // Position text at the bottom of the box

            int text_clip_x1 = box_x + text_padding;
            int text_clip_y1 = box_y + IconSize::LARGE_HEIGHT + 2; // Clip below icon
            int text_clip_x2 = box_x + itemW - text_padding;
            int text_clip_y2 = box_y + itemH - 1;

            if (text_clip_y1 < clip_y_start) text_clip_y1 = clip_y_start;
            if (text_clip_y2 > 63) text_clip_y2 = 63;

            if (text_clip_y1 < text_clip_y2) {
                 display.setClipWindow(text_clip_x1, text_clip_y1, text_clip_x2, text_clip_y2);
            }

            if (isSelected) {
                updateMarquee(marqueeActive_, marqueePaused_, marqueeScrollLeft_, 
                              marqueePauseStartTime_, lastMarqueeTime_, marqueeOffset_, 
                              marqueeText_, marqueeTextLenPx_, menuItems_[i].label, innerW, display);
                
                if (marqueeActive_) {
                    display.drawStr(box_x + text_padding + (int)marqueeOffset_, text_y, marqueeText_);
                } else {
                    int textW = display.getStrWidth(menuItems_[i].label);
                    display.drawStr(box_x + (itemW - textW) / 2, text_y, menuItems_[i].label);
                }
            } else {
                char* truncated = truncateText(menuItems_[i].label, innerW, display);
                int textW = display.getStrWidth(truncated);
                display.drawStr(box_x + (itemW - textW) / 2, text_y, truncated);
            }

            display.setClipWindow(0, clip_y_start, 127, 63);
        }
    }

    display.setMaxClipWindow();
    display.setDrawColor(1);
}