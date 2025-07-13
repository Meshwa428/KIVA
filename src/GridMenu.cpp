#include "GridMenu.h"
#include "App.h"
#include "UI_Utils.h"
#include <Arduino.h>

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
    else if (title == "Jammer") menuType_ = MenuType::JAMMING_TOOLS_GRID; // <-- ADD
    else menuType_ = MenuType::NONE;
}
void GridMenu::onEnter(App *app)
{
    selectedIndex_ = 0;
    targetGridScrollOffset_Y_ = 0;
    currentGridScrollOffset_Y_anim_ = 0;
    startGridAnimation();
    marqueeScrollLeft_ = true; // Reset on enter
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
        for (size_t i = 0; i < menuItems_.size() && i < MAX_GRID_ITEMS; ++i)
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
        case InputEvent::ENCODER_CW:
        case InputEvent::BTN_RIGHT_PRESS:
            scroll(1);
            break;
        case InputEvent::ENCODER_CCW:
        case InputEvent::BTN_LEFT_PRESS:
            scroll(-1);
            break;
        case InputEvent::BTN_UP_PRESS:
            scroll(-columns_);
            break;
        case InputEvent::BTN_DOWN_PRESS:
            scroll(columns_);
            break;
        case InputEvent::BTN_ENCODER_PRESS:
        case InputEvent::BTN_OK_PRESS:
            {
                if (selectedIndex_ >= menuItems_.size()) break;
                const auto& selected = menuItems_[selectedIndex_];

                // --- NEW GENERIC LOGIC ---
                if (selected.action) {
                    // If an action is defined, execute it.
                    selected.action(app);
                } else {
                    // Otherwise, navigate to the target menu.
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
    selectedIndex_ += direction;

    if (selectedIndex_ >= (int)menuItems_.size()) {
        selectedIndex_ = selectedIndex_ % menuItems_.size();
    }
    if (selectedIndex_ < 0) {
        selectedIndex_ = (selectedIndex_ % menuItems_.size() + menuItems_.size()) % menuItems_.size();
    }
    
    if (abs(direction) > 1) { // For UP/DOWN
        int new_col = oldIndex % columns_;
        // Check if the new row has a valid item in this column. If not, stay put.
        int targetIndex = (selectedIndex_ / columns_) * columns_ + new_col;
        if(targetIndex < menuItems_.size()) {
             selectedIndex_ = targetIndex;
        } else {
            selectedIndex_ = oldIndex;
        }
    }


    const int itemRowHeight = 18 + 4;
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
    marqueeScrollLeft_ = true; // Reset on scroll
}

void GridMenu::startGridAnimation()
{
    gridAnimatingIn_ = true;
    unsigned long currentTime = millis();
    for (size_t i = 0; i < MAX_GRID_ITEMS; ++i)
    {
        if (i < menuItems_.size())
        {
            gridItemScale_[i] = 0.0f;
            int row = i / columns_;
            int col = i % columns_;
            gridItemAnimStartTime_[i] = currentTime + (unsigned long)((row * 1.5f + col) * GRID_ANIM_STAGGER_DELAY);
        }
        else
        {
            gridItemScale_[i] = 0.0f;
        }
    }
}

void GridMenu::draw(App* app, U8G2& display) {
    const int itemBaseW = (128 - (columns_ + 1) * 4) / columns_;
    const int itemBaseH = 18;
    const int gridStartY = STATUS_BAR_H + 1 + 4;

    display.setFont(u8g2_font_5x7_tf);
    display.setClipWindow(0, STATUS_BAR_H + 1, 127, 63);

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
        
        if (box_y + itemH <= STATUS_BAR_H + 1 || box_y >= 64) continue;
        
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
            int innerW = itemW - 4;
            
            // ** MODIFIED: Pass all marquee state variables **
            updateMarquee(marqueeActive_, marqueePaused_, marqueeScrollLeft_, 
                          marqueePauseStartTime_, lastMarqueeTime_, marqueeOffset_, 
                          marqueeText_, marqueeTextLenPx_, menuItems_[i].label, innerW, display);
            
            display.setClipWindow(box_x+1, box_y+1, box_x+itemW-1, box_y+itemH-1);
            int text_y = box_y + (itemH - (display.getAscent() - display.getDescent())) / 2 + display.getAscent();
            if (isSelected && marqueeActive_) {
                display.drawStr(box_x + 2 + (int)marqueeOffset_, text_y, marqueeText_);
            } else {
                char* truncated = truncateText(menuItems_[i].label, innerW, display);
                int textW = display.getStrWidth(truncated);
                display.drawStr(box_x + (itemW - textW) / 2, text_y, truncated);
            }
            display.setClipWindow(0, STATUS_BAR_H + 1, 127, 63);
        }
    }
    display.setDrawColor(1);
    display.setMaxClipWindow();
}