#include "ListMenu.h"
#include "App.h"
#include "UI_Utils.h"
#include "Event.h"
#include "EventDispatcher.h"

ListMenu::ListMenu(std::string title, MenuType menuType, IListMenuDataSource* dataSource) : 
    title_(title),
    menuType_(menuType),
    dataSource_(dataSource),
    selectedIndex_(0),
    totalItems_(0),
    marqueeActive_(false),
    marqueeScrollLeft_(true)
{}

void ListMenu::reloadData(App* app, bool resetSelection) {
    if (!dataSource_) return;
    
    totalItems_ = dataSource_->getNumberOfItems(app);
    animation_.resize(totalItems_); // Resize must happen before startIntro

    if (resetSelection) {
        selectedIndex_ = 0;
    }
    // Clamp selectedIndex in case the list size changed while we were away
    if (totalItems_ > 0 && selectedIndex_ >= totalItems_) {
        selectedIndex_ = totalItems_ - 1;
    } else if (totalItems_ == 0) {
        selectedIndex_ = 0;
    }

    marqueeActive_ = false;
    marqueeScrollLeft_ = true;
    animation_.startIntro(selectedIndex_, totalItems_);
}

void ListMenu::onEnter(App* app, bool isForwardNav) {
    EventDispatcher::getInstance().subscribe(EventType::APP_INPUT, this);
    if (!dataSource_) return;
    dataSource_->onEnter(app, this, isForwardNav);
    reloadData(app, isForwardNav); // Pass the flag to reloadData
}

void ListMenu::onUpdate(App* app) {
    if (animation_.update()) {
        app->requestRedraw();
    }
    if (dataSource_) {
        dataSource_->onUpdate(app, this);
    }
}

void ListMenu::onExit(App* app) {
    EventDispatcher::getInstance().unsubscribe(EventType::APP_INPUT, this);
    if (dataSource_) {
        dataSource_->onExit(app, this);
    }
    marqueeActive_ = false;
}

void ListMenu::handleInput(InputEvent event, App* app) {
    if (!dataSource_) return;

    if (event == InputEvent::BTN_BACK_PRESS) {
        if (!dataSource_->onBackPress(app, this)) {
            EventDispatcher::getInstance().publish(NavigateBackEvent());
        }
        return;
    }

    if (totalItems_ == 0) {
        return;
    }
    
    // --- NEW: Handle slider adjustments ---
    const MenuItem* item = dataSource_->getItem(selectedIndex_);
    if (item && item->isInteractive && item->adjustValue) {
        if (event == InputEvent::BTN_LEFT_PRESS || event == InputEvent::ENCODER_CCW) {
            item->adjustValue(app, -1); // -1 for decrease
            return; // Consume the event
        }
        if (event == InputEvent::BTN_RIGHT_PRESS || event == InputEvent::ENCODER_CW) {
            item->adjustValue(app, 1); // 1 for increase
            return; // Consume the event
        }
    }
    // --- END NEW ---

    switch(event) {
        case InputEvent::ENCODER_CW:
        case InputEvent::BTN_DOWN_PRESS:
            scroll(1);
            break;
        case InputEvent::ENCODER_CCW:
        case InputEvent::BTN_UP_PRESS:
            scroll(-1);
            break;
        case InputEvent::BTN_ENCODER_PRESS:
        case InputEvent::BTN_OK_PRESS:
            dataSource_->onItemSelected(app, this, selectedIndex_);
            break;
        default: break;
    }
}

void ListMenu::scroll(int direction) {
    if (totalItems_ == 0) return;
    selectedIndex_ += direction;
    if (selectedIndex_ < 0) selectedIndex_ = totalItems_ - 1;
    else if (selectedIndex_ >= totalItems_) selectedIndex_ = 0;
    
    animation_.setTargets(selectedIndex_, totalItems_);
    marqueeActive_ = false;
    marqueeScrollLeft_ = true;
}

void ListMenu::draw(App* app, U8G2& display) {
    if (!dataSource_) return;

    if (totalItems_ == 0) {
        if (!dataSource_->drawCustomEmptyMessage(app, display)) {
            const char *msg = "No items";
            display.setFont(u8g2_font_6x10_tf);
            display.drawStr((display.getDisplayWidth() - display.getStrWidth(msg)) / 2, 38, msg);
        }
        return;
    }

    const int list_start_y = STATUS_BAR_H + 1;
    const int item_h = 18;

    display.setClipWindow(0, list_start_y, 127, 63);
    display.setFont(u8g2_font_6x10_tf);

    for (int i = 0; i < totalItems_; ++i) {
        int item_center_y_rel = (int)animation_.itemOffsetY[i];
        float scale = animation_.itemScale[i];
        if (scale <= 0.01f) continue;
        
        int item_center_y_abs = (list_start_y + (63 - list_start_y) / 2) + item_center_y_rel;
        int item_top_y = item_center_y_abs - item_h / 2;

        if (item_top_y > 63 || item_top_y + item_h < list_start_y) continue;
        
        bool isSelected = (i == selectedIndex_);

        if (isSelected) {
            drawRndBox(display, 2, item_top_y, 124, item_h, 2, true);
        }

        dataSource_->drawItem(app, display, this, i, 2, item_top_y, 124, item_h, isSelected);
    }

    display.setDrawColor(1);
    display.setMaxClipWindow();
}

void ListMenu::updateAndDrawText(U8G2& display, const char* text, int x, int y, int availableWidth, bool isSelected) {
    if (isSelected) {
        updateMarquee(marqueeActive_, marqueePaused_, marqueeScrollLeft_, 
                      marqueePauseStartTime_, lastMarqueeTime_, marqueeOffset_, 
                      marqueeText_, marqueeTextLenPx_, text, availableWidth, display);

        display.setClipWindow(x, y - 8, x + availableWidth, y + 8);
        
        if (marqueeActive_) {
            display.drawStr(x + (int)marqueeOffset_, y, marqueeText_);
        } else {
            display.drawStr(x, y, text);
        }
        display.setMaxClipWindow();
    } else {
        char* truncated = truncateText(text, availableWidth, display);
        display.drawStr(x, y, truncated);
    }
}