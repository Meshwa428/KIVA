#include "SplitSelectionMenu.h"
#include "App.h"
#include "UI_Utils.h"
#include <algorithm>
#include <vector>
#include <sstream>

SplitSelectionMenu::SplitSelectionMenu(std::string title, MenuType menuType, const std::vector<MenuItem>& items)
    : title_(title),
      menuType_(menuType),
      menuItems_(items),
      selectedIndex_(0), 
      isAnimatingIn_(false),
      marqueeActive_(false),
      marqueeScrollLeft_(true)
{
    for (int i = 0; i < 3; ++i) { 
        panelCurrentOffsetX_[i] = 0.0f;
        panelTargetOffsetX_[i] = 0.0f;
        panelCurrentScale_[i] = 0.0f;
        panelTargetScale_[i] = 0.0f;
    }
}

void SplitSelectionMenu::onEnter(App* app) {
    selectedIndex_ = 0;
    isAnimatingIn_ = true;
    marqueeActive_ = false;
    marqueeScrollLeft_ = true;

    for (size_t i = 0; i < menuItems_.size() && i < 3; ++i) {
        panelCurrentScale_[i] = 0.0f;
        if (menuItems_.size() < 3 || i == 1) { // Center item for 2 or 3 item layout
            panelCurrentOffsetX_[i] = 0;
        } else {
            panelCurrentOffsetX_[i] = (i == 0) ? -64.f : 64.f;
        }
    }
    scroll(0);
}

void SplitSelectionMenu::onUpdate(App* app) {
    bool isAnimating = false;
    for (size_t i = 0; i < menuItems_.size() && i < 3; ++i) {
        float offsetDiff = panelTargetOffsetX_[i] - panelCurrentOffsetX_[i];
        float scaleDiff = panelTargetScale_[i] - panelCurrentScale_[i];
        
        if (std::abs(offsetDiff) > 0.1f) {
            panelCurrentOffsetX_[i] += offsetDiff * 15.f * 0.016f;
            isAnimating = true;
        } else {
            panelCurrentOffsetX_[i] = panelTargetOffsetX_[i];
        }

        if (std::abs(scaleDiff) > 0.01f) {
            panelCurrentScale_[i] += scaleDiff * 20.f * 0.016f;
            isAnimating = true;
        } else {
            panelCurrentScale_[i] = panelTargetScale_[i];
        }
    }
    if (isAnimatingIn_ && !isAnimating) {
        isAnimatingIn_ = false;
    }
}

void SplitSelectionMenu::onExit(App* app) {}

void SplitSelectionMenu::handleInput(App* app, InputEvent event) {
    switch (event) {
        case InputEvent::ENCODER_CW:
        case InputEvent::BTN_RIGHT_PRESS:
            scroll(1);
            break;
        case InputEvent::ENCODER_CCW:
        case InputEvent::BTN_LEFT_PRESS:
            scroll(-1);
            break;
        case InputEvent::BTN_OK_PRESS:
        case InputEvent::BTN_ENCODER_PRESS:
            {
                const auto& selected = menuItems_[selectedIndex_];
                if (selected.targetMenu == MenuType::BACK) {
                    app->changeMenu(MenuType::BACK);
                } else if (selected.action) {
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


void SplitSelectionMenu::scroll(int direction) {
    if (menuItems_.empty()) return;
    
    selectedIndex_ += direction;
    if (selectedIndex_ < 0) selectedIndex_ = menuItems_.size() - 1;
    else if (selectedIndex_ >= (int)menuItems_.size()) selectedIndex_ = 0;

    marqueeActive_ = false;
    marqueeScrollLeft_ = true;

    for (size_t i = 0; i < menuItems_.size() && i < 3; ++i) {
        panelTargetOffsetX_[i] = 0;
        panelTargetScale_[i] = (i == selectedIndex_) ? 1.0f : 0.85f;
    }
}

void SplitSelectionMenu::draw(App* app, U8G2& display) {
    int numItems = menuItems_.size();
    if (numItems == 0) return;
    
    int panelWidth = (128 - (numItems + 1) * 4) / numItems;
    int panelHeight = 63 - (STATUS_BAR_H + 4);
    int panelY = STATUS_BAR_H + 3;

    for (int i = 0; i < numItems; ++i) {
        float scale = (i < 3) ? panelCurrentScale_[i] : 1.0f;
        float offsetX = (i < 3) ? panelCurrentOffsetX_[i] : 0.0f;
        if (scale <= 0.05f) continue;
        
        int w = panelWidth * scale;
        int h = panelHeight * scale;
        
        int x_base = 4 + i * (panelWidth + 4);
        int x = x_base + (panelWidth - w) / 2 + offsetX;
        int y = panelY + (panelHeight - h) / 2;

        bool isSelected = (i == selectedIndex_);
        
        display.setDrawColor(1);
        drawRndBox(display, x, y, w, h, 3, isSelected);

        display.setDrawColor(isSelected ? 0 : 1);
        display.setFont(u8g2_font_6x10_tf);
        
        const char* label = menuItems_[i].label;
        const int textPaddingX = 3;
        const int topPadding = 8;
        const int gap_h = 4;

        // 1. ANCHOR: Icon position is fixed and padded from the top.
        int iconX = x + (w - IconSize::LARGE_WIDTH) / 2;
        int iconY = y + topPadding;
        drawCustomIcon(display, iconX, iconY, menuItems_[i].icon);

        // 2. DEFINE: Text area is everything below the icon.
        int textAreaY = iconY + IconSize::LARGE_HEIGHT + gap_h;
        int textAreaW = w - 2 * textPaddingX;
        int textAreaH = (y + h) - textAreaY - (topPadding / 2);
        
        // --- THE FIX: The faulty check that was here is now REMOVED ---
        
        // 3. LOGIC: Check if the text overflows its available width.
        bool textOverflows = display.getStrWidth(label) > textAreaW;
        int line_height = display.getAscent() - display.getDescent();
        int text_y_baseline = textAreaY + (textAreaH - line_height) / 2 + display.getAscent();

        if (isSelected) {
            if (textOverflows) {
                // BEHAVIOR: SELECTED & OVERFLOWING -> MARQUEE
                updateMarquee(marqueeActive_, marqueePaused_, marqueeScrollLeft_, 
                              marqueePauseStartTime_, lastMarqueeTime_, marqueeOffset_, 
                              marqueeText_, marqueeTextLenPx_, label, textAreaW, display);
                
                display.setClipWindow(x + textPaddingX, y, x + w - textPaddingX, y + h);
                display.drawStr(x + textPaddingX + (int)marqueeOffset_, text_y_baseline, marqueeText_);
                display.setMaxClipWindow();
            } else {
                // BEHAVIOR: SELECTED & FITS -> DRAW CENTERED
                int text_width = display.getStrWidth(label);
                display.drawStr(x + (w - text_width) / 2, text_y_baseline, label);
            }
        } else {
            // BEHAVIOR: NOT SELECTED
            char* text_to_draw = (char*)label;
            if (textOverflows) {
                // BEHAVIOR: NOT SELECTED & OVERFLOWING -> TRUNCATE
                text_to_draw = truncateText(label, textAreaW, display);
            }
            // BEHAVIOR: DRAW CENTERED (either full text or truncated)
            int text_width = display.getStrWidth(text_to_draw);
            display.drawStr(x + (w - text_width) / 2, text_y_baseline, text_to_draw);
        }
    }
    display.setDrawColor(1);
}