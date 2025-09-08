#include "CarouselMenu.h"
#include "App.h"
#include "UI_Utils.h"
#include "Event.h"
#include "EventDispatcher.h"
#include <Arduino.h>

// --- CONSTRUCTOR MODIFIED ---
CarouselMenu::CarouselMenu(std::string title, MenuType menuType, std::vector<MenuItem> items) :
    title_(title),
    menuItems_(items),
    menuType_(menuType), // Set type from parameter
    selectedIndex_(0),
    marqueeActive_(false),
    marqueeScrollLeft_(true)
{
    // REMOVED all the fragile "if (title == ...)" logic
}

void CarouselMenu::onEnter(App* app, bool isForwardNav) {
    EventDispatcher::getInstance().subscribe(EventType::APP_INPUT, this);
    if (isForwardNav) {
        selectedIndex_ = 0;
        marqueeScrollLeft_ = true; // Reset marquee direction
    }
    animation_.resize(menuItems_.size());
    animation_.init();
    animation_.setTargets(selectedIndex_, menuItems_.size());
}

void CarouselMenu::onUpdate(App* app) {
    animation_.update();
}

void CarouselMenu::onExit(App* app) {
    EventDispatcher::getInstance().unsubscribe(EventType::APP_INPUT, this);
    marqueeActive_ = false;
}


void CarouselMenu::handleInput(InputEvent event, App* app) {
    switch(event) {
        case InputEvent::ENCODER_CW:
        case InputEvent::BTN_RIGHT_PRESS:
            scroll(1);
            break;
        case InputEvent::ENCODER_CCW:
        case InputEvent::BTN_LEFT_PRESS:
            scroll(-1);
            break;
        case InputEvent::BTN_ENCODER_PRESS:
        case InputEvent::BTN_OK_PRESS:
            {
                if (selectedIndex_ >= menuItems_.size()) break;
                const MenuItem& selected = menuItems_[selectedIndex_];

                if (selected.action) {
                    selected.action(app);
                } else {
                    EventDispatcher::getInstance().publish(NavigateToMenuEvent(selected.targetMenu));
                }
            }
            break;
        case InputEvent::BTN_BACK_PRESS:
            EventDispatcher::getInstance().publish(NavigateBackEvent());
            break;
        default:
            break;
    }
}

void CarouselMenu::scroll(int direction) {
    if (menuItems_.empty()) return;
    selectedIndex_ += direction;
    if (selectedIndex_ < 0) {
        selectedIndex_ = menuItems_.size() - 1;
    } else if (selectedIndex_ >= menuItems_.size()) {
        selectedIndex_ = 0;
    }
    animation_.setTargets(selectedIndex_, menuItems_.size());
    marqueeActive_ = false;
    marqueeScrollLeft_ = true; // Reset on scroll
}

void CarouselMenu::draw(App* app, U8G2& display) {
    const int drawable_area_y_start = STATUS_BAR_H;
    const int drawable_area_height = display.getDisplayHeight() - drawable_area_y_start;
    const int carousel_center_y = drawable_area_y_start + (drawable_area_height / 2);

    const int screen_center_x = 64;
    const int card_padding = 3;
    const int icon_margin_top = 3;
    const int text_margin_from_icon = 2;

    display.setFont(u8g2_font_6x10_tf);
    HardwareManager& hw = app->getHardwareManager();

    for (size_t i = 0; i < menuItems_.size(); i++) {
        float scale = animation_.itemScale[i];
        if (scale <= 0.05f) continue;

        float offsetX = animation_.itemOffsetX[i];
        int card_w = (int)(animation_.cardBaseW * scale);
        int card_h = (int)(animation_.cardBaseH * scale);
        int card_x = screen_center_x + (int)offsetX - (card_w / 2);
        int card_y = carousel_center_y - (card_h / 2);
        bool isSelected = (i == selectedIndex_);

        char itemDisplayTextBuffer[20];
        const char* itemTextToDisplay = menuItems_[i].label;

        // REMOVED logic for 'Utilities' carousel as it no longer exists
        
        if (isSelected) {
            display.setDrawColor(1);
            drawRndBox(display, card_x, card_y, card_w, card_h, 3, true);
            display.setDrawColor(0);

            int icon_x = card_x + (card_w - IconSize::LARGE_WIDTH) / 2;
            int icon_y = card_y + icon_margin_top;
            drawCustomIcon(display, icon_x, icon_y, menuItems_[i].icon);
            
            int text_area_y = card_y + icon_margin_top + IconSize::LARGE_HEIGHT + text_margin_from_icon;
            int text_area_h = card_h - (text_area_y - card_y) - card_padding;
            int text_y_base = text_area_y + (text_area_h - (display.getAscent() - display.getDescent())) / 2 + display.getAscent() - 1;

            int inner_w = card_w - 2 * card_padding;
            
            updateMarquee(marqueeActive_, marqueePaused_, marqueeScrollLeft_, 
                          marqueePauseStartTime_, lastMarqueeTime_, marqueeOffset_, 
                          marqueeText_, marqueeTextLenPx_, itemTextToDisplay, inner_w, display);

            display.setClipWindow(card_x + card_padding, text_area_y, card_x + card_w - card_padding, card_y + card_h - card_padding);
            if (marqueeActive_) {
                display.drawStr(card_x + card_padding + (int)marqueeOffset_, text_y_base, marqueeText_);
            } else {
                int text_w = display.getStrWidth(itemTextToDisplay);
                display.drawStr(card_x + (card_w - text_w) / 2, text_y_base, itemTextToDisplay);
            }
            display.setMaxClipWindow();
        } else {
            display.setDrawColor(1);
            drawRndBox(display, card_x, card_y, card_w, card_h, 2, false);

            if (scale > 0.5) {
                display.setFont(u8g2_font_5x7_tf);
                char* truncated = truncateText(itemTextToDisplay, card_w - 2 * card_padding, display);
                int text_w = display.getStrWidth(truncated);
                int text_y = card_y + (card_h - (display.getAscent() - display.getDescent())) / 2 + display.getAscent();
                display.drawStr(card_x + (card_w - text_w) / 2, text_y, truncated);
                display.setFont(u8g2_font_6x10_tf);
            }
        }
    }
    display.setDrawColor(1);
}