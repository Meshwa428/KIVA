#include "ListMenu.h"
#include "App.h"
#include "UI_Utils.h"

ListMenu::ListMenu(std::string title, MenuType menuType, IListMenuDataSource* dataSource) : 
    title_(title),
    menuType_(menuType),
    dataSource_(dataSource),
    selectedIndex_(0),
    totalItems_(0),
    marqueeActive_(false),
    marqueeScrollLeft_(true)
{}

void ListMenu::reloadData(App* app) {
    if (!dataSource_) return;
    
    totalItems_ = dataSource_->getNumberOfItems(app);
    selectedIndex_ = 0;
    marqueeActive_ = false;
    marqueeScrollLeft_ = true;
    animation_.init();
    animation_.startIntro(selectedIndex_, totalItems_);
}

void ListMenu::onEnter(App* app) {
    if (!dataSource_) return;
    dataSource_->onEnter(app, this);
    reloadData(app);
}

void ListMenu::onUpdate(App* app) {
    animation_.update();
    if (dataSource_) {
        dataSource_->onUpdate(app, this);
    }
}

void ListMenu::onExit(App* app) {
    if (dataSource_) {
        dataSource_->onExit(app, this);
    }
    marqueeActive_ = false;
}

void ListMenu::handleInput(App* app, InputEvent event) {
    if (!dataSource_) return;

    // --- FIX: Allow back button even if list is empty (e.g. while scanning) ---
    if (totalItems_ == 0) {
        if (event == InputEvent::BTN_BACK_PRESS) {
            app->changeMenu(MenuType::BACK);
        }
        return;
    }

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
        case InputEvent::BTN_BACK_PRESS:
            app->changeMenu(MenuType::BACK);
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

    // --- FIX: Handle empty/loading state ---
    if (totalItems_ == 0) {
        // Allow data source to draw a custom message (e.g., "Scanning...")
        if (!dataSource_->drawCustomEmptyMessage(app, display)) {
            // If it doesn't, draw the default message
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
        if (i >= MAX_ANIM_ITEMS) break;

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

        // Delegate drawing to the data source, passing a non-const 'this'
        dataSource_->drawItem(app, display, this, i, 2, item_top_y, 124, item_h, isSelected);
    }

    display.setDrawColor(1);
    display.setMaxClipWindow();
}

// --- NEW: Implementation of the marquee helper function ---
void ListMenu::updateAndDrawText(U8G2& display, const char* text, int x, int y, int availableWidth, bool isSelected) {
    if (isSelected) {
        updateMarquee(marqueeActive_, marqueePaused_, marqueeScrollLeft_, 
                      marqueePauseStartTime_, lastMarqueeTime_, marqueeOffset_, 
                      marqueeText_, marqueeTextLenPx_, text, availableWidth, display);

        display.setClipWindow(x, y - 8, x + availableWidth, y + 8); // Clip tightly around the text line
        
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