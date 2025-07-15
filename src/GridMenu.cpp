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
    marqueeScrollLeft_(true) // Already initialized correctly, but good to confirm
{
    // Infer MenuType from title
    if (title == "WiFi Tools") menuType_ = MenuType::WIFI_TOOLS_GRID;
    else if (title == "Update") menuType_ = MenuType::FIRMWARE_UPDATE_GRID;
    else if (title == "Jammer") menuType_ = MenuType::JAMMING_TOOLS_GRID;
    else menuType_ = MenuType::NONE;
}

void GridMenu::onEnter(App *app)
{
    selectedIndex_ = 0;
    targetGridScrollOffset_Y_ = 0;
    currentGridScrollOffset_Y_anim_ = 0;
    startGridAnimation();
    
    // --- FIX: Reset marquee state on enter ---
    marqueeActive_ = false;
    marqueeScrollLeft_ = true;
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
    int targetIndex = selectedIndex_ + direction;

    // Handle left/right wrapping on the same row
    if (abs(direction) == 1) {
        int currentRow = selectedIndex_ / columns_;
        int startOfRow = currentRow * columns_;
        int endOfRow = startOfRow + columns_ - 1;
        
        // Find the actual last item in the row, which might be less than endOfRow
        int lastItemInRow = startOfRow;
        for(int i = startOfRow; i < menuItems_.size() && i <= endOfRow; ++i) {
            lastItemInRow = i;
        }

        if (targetIndex < startOfRow) targetIndex = lastItemInRow;
        if (targetIndex > lastItemInRow) targetIndex = startOfRow;
    }
    
    // Clamp vertical movement
    if (targetIndex < 0) targetIndex = oldIndex;
    if (targetIndex >= (int)menuItems_.size()) targetIndex = oldIndex;
    
    selectedIndex_ = targetIndex;

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
    
    // --- FIX: Reset marquee state on every scroll ---
    marqueeActive_ = false;
    marqueeScrollLeft_ = true;
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
    
    // --- STEP 1: Set the main clipping area for the entire grid ---
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
            const int text_padding = 2;
            int innerW = itemW - (2 * text_padding);
            int text_y = box_y + (itemH - (display.getAscent() - display.getDescent())) / 2 + display.getAscent();

            // --- THE CRITICAL FIX ---
            // We calculate the desired clip window for the text, then clamp its
            // vertical coordinates to the main visible area. This prevents the
            // clip window itself from being defined outside the visible screen.
            int text_clip_x1 = box_x + text_padding;
            int text_clip_y1 = box_y + 1;
            int text_clip_x2 = box_x + itemW - text_padding;
            int text_clip_y2 = box_y + itemH - 1;

            // Clamp the vertical clipping coordinates
            if (text_clip_y1 < clip_y_start) text_clip_y1 = clip_y_start;
            if (text_clip_y2 > 63) text_clip_y2 = 63;

            // Only set the clip window if it's a valid area
            if (text_clip_y1 < text_clip_y2) {
                 display.setClipWindow(text_clip_x1, text_clip_y1, text_clip_x2, text_clip_y2);
            }
            // --- END OF CRITICAL FIX ---


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

            // Reset the clip window back to the main grid area for the next item.
            display.setClipWindow(0, clip_y_start, 127, 63);
        }
    }

    display.setMaxClipWindow();
    display.setDrawColor(1);
}