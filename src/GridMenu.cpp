#include "GridMenu.h"
#include "App.h"
#include "UI_Utils.h"
#include "Event.h"
#include "EventDispatcher.h"
#include <Arduino.h>
#include <algorithm> // For std::min

GridMenu::GridMenu(std::string title, MenuType menuType, std::vector<MenuItem> items, int columns) : 
    title_(title),
    menuItems_(items),
    menuType_(menuType),
    columns_(columns),
    selectedIndex_(0),
    animation_(),
    marqueeActive_(false),
    marqueeScrollLeft_(true),
    isScrolling_(false),
    scrollDirection_(0),
    pressStartTime_(0),
    lastScrollTime_(0)
{}

void GridMenu::onEnter(App *app, bool isForwardNav)
{
    EventDispatcher::getInstance().subscribe(EventType::APP_INPUT, this);
    isScrolling_ = false;

    animation_.resize(menuItems_.size()); // Always ensure vectors are sized correctly.

    if (isForwardNav) {
        // Forward Navigation: Reset everything and play the intro from the top.
        selectedIndex_ = 0;
        marqueeScrollLeft_ = true;
        animation_.init(); // Resets scroll offsets and scales.
        animation_.startIntro(menuItems_.size(), columns_);
    } else {
        // Backward Navigation: Preserve selection and animate into the correct view.
        animation_.init(); // Reset scales/timers for a fresh animation.

        // Calculate the correct scroll offset to make the selected item visible.
        const int itemRowHeight = 28 + 4; // Item height + vertical padding
        const int gridVisibleAreaH = 64 - (STATUS_BAR_H + 1) - 4;
        const int visibleRows = gridVisibleAreaH / itemRowHeight;
        int currentSelectionRow = selectedIndex_ / columns_;

        // Aim to center the selected row, but clamp the scroll position to avoid empty space.
        int targetTopRow = currentSelectionRow - (visibleRows / 2);
        if (targetTopRow < 0) {
            targetTopRow = 0;
        }
        int totalRows = (menuItems_.size() + columns_ - 1) / columns_;
        int maxTopRow = totalRows - visibleRows;
        if (maxTopRow < 0) {
            maxTopRow = 0; // Happens if all items fit on screen
        }
        if (targetTopRow > maxTopRow) {
            targetTopRow = maxTopRow;
        }
        
        animation_.setScrollTarget(targetTopRow * itemRowHeight); // Set the scroll destination.
        animation_.startIntro(menuItems_.size(), columns_);      // Start the staggered item fade-in animation.
    }
    
    marqueeActive_ = false; // Reset marquee for both cases.
}

void GridMenu::onUpdate(App *app)
{
    if (animation_.update()) {
        app->requestRedraw();
    }
    
    // Continuous scrolling
    if (isScrolling_) {
        unsigned long currentTime = millis();
        unsigned long holdDuration = currentTime - pressStartTime_;
        
        // Use a simple, two-stage acceleration
        unsigned long repeatInterval = (holdDuration > 500) ? 100 : 200;

        if (currentTime - lastScrollTime_ > repeatInterval) {
            scroll(scrollDirection_);
            lastScrollTime_ = currentTime;
        }
    }
}

void GridMenu::onExit(App *app)
{
    EventDispatcher::getInstance().unsubscribe(EventType::APP_INPUT, this);
    marqueeActive_ = false;
    isScrolling_ = false; // Reset state on exit
}

void GridMenu::handleInput(InputEvent event, App* app) {
    switch (event) {
        // Discrete Scrolling (Encoder)
        case InputEvent::ENCODER_CW:
            scroll(1);
            break;
        case InputEvent::ENCODER_CCW:
            scroll(-1);
            break;

        // Continuous Scrolling (Buttons) - Start
        case InputEvent::BTN_UP_PRESS:
            if (isScrolling_) break;
            isScrolling_ = true; scrollDirection_ = -columns_; pressStartTime_ = millis(); lastScrollTime_ = millis();
            scroll(scrollDirection_);
            break;
        case InputEvent::BTN_DOWN_PRESS:
            if (isScrolling_) break;
            isScrolling_ = true; scrollDirection_ = columns_; pressStartTime_ = millis(); lastScrollTime_ = millis();
            scroll(scrollDirection_);
            break;
        case InputEvent::BTN_RIGHT_PRESS:
            if (isScrolling_) break;
            isScrolling_ = true; scrollDirection_ = 1; pressStartTime_ = millis(); lastScrollTime_ = millis();
            scroll(scrollDirection_);
            break;
        case InputEvent::BTN_LEFT_PRESS:
            if (isScrolling_) break;
            isScrolling_ = true; scrollDirection_ = -1; pressStartTime_ = millis(); lastScrollTime_ = millis();
            scroll(scrollDirection_);
            break;

        // Continuous Scrolling (Buttons) - Stop
        case InputEvent::BTN_UP_RELEASE:
        case InputEvent::BTN_DOWN_RELEASE:
        case InputEvent::BTN_RIGHT_RELEASE:
        case InputEvent::BTN_LEFT_RELEASE:
            isScrolling_ = false;
            break;

        // Item Selection
        case InputEvent::BTN_ENCODER_PRESS:
        case InputEvent::BTN_OK_PRESS:
            {
                if (selectedIndex_ >= (int)menuItems_.size()) break;
                const auto& selected = menuItems_[selectedIndex_];

                if (selected.action) {
                    selected.action(app);
                } else {
                    EventDispatcher::getInstance().publish(NavigateToMenuEvent(selected.targetMenu));
                }
            }
            break;

        // Back Navigation
        case InputEvent::BTN_BACK_PRESS:
            EventDispatcher::getInstance().publish(NavigateBackEvent());
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

    const int itemRowHeight = 28 + 4; 
    const int gridVisibleAreaH = 64 - (STATUS_BAR_H + 1) - 4;
    const int visibleRows = gridVisibleAreaH / itemRowHeight;

    int currentSelectionRow = selectedIndex_ / columns_;
    int topVisibleRow = (int)(animation_.targetScrollOffset_Y / itemRowHeight);

    if (currentSelectionRow < topVisibleRow) {
        animation_.setScrollTarget(currentSelectionRow * itemRowHeight);
    } else if (currentSelectionRow >= topVisibleRow + visibleRows) {
        animation_.setScrollTarget((currentSelectionRow - visibleRows + 1) * itemRowHeight);
    }
    
    marqueeActive_ = false;
    marqueeScrollLeft_ = true;
}

void GridMenu::draw(App* app, U8G2& display) {
    const int itemBaseW = (128 - (columns_ + 1) * 4) / columns_;
    const int itemBaseH = 28;
    const int gridStartY = STATUS_BAR_H + 1 + 4;
    
    const int clip_y_start = STATUS_BAR_H + 1;
    display.setClipWindow(0, clip_y_start, 127, 63);

    display.setFont(u8g2_font_5x7_tf);

    for (size_t i = 0; i < menuItems_.size(); ++i) {
        int row = i / columns_;
        int col = i % columns_;

        float scale = animation_.isAnimatingIn ? animation_.itemScale[i] : 1.0f;
        int itemW = (int)(itemBaseW * scale);
        int itemH = (int)(itemBaseH * scale);
        if (itemW < 1 || itemH < 1) continue;

        int box_x = 4 + col * (itemBaseW + 4) + (itemBaseW - itemW) / 2;
        int box_y_noscroll = gridStartY + row * (itemBaseH + 4) + (itemBaseH - itemH) / 2;
        int box_y = box_y_noscroll - (int)animation_.currentScrollOffset_Y;
        
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
            int icon_y = box_y + 2;
            int icon_x = box_x + (itemW - IconSize::LARGE_WIDTH) / 2;
            drawCustomIcon(display, icon_x, icon_y, menuItems_[i].icon);

            const int text_padding = 2;
            int innerW = itemW - (2 * text_padding);
            int text_y = box_y + itemH - 3;

            int text_clip_x1 = box_x + text_padding;
            int text_clip_y1 = box_y + IconSize::LARGE_HEIGHT + 2;
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
                              marqueeText_, sizeof(marqueeText_), marqueeTextLenPx_, menuItems_[i].label, innerW, display);
                
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