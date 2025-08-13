#include "FirmwareListMenu.h"
#include "App.h"
#include "OtaManager.h"
#include "UI_Utils.h"

FirmwareListMenu::FirmwareListMenu() : 
    selectedIndex_(0),
    marqueeActive_(false),      // <-- Initialize Marquee
    marqueeScrollLeft_(true)    // <-- Initialize Marquee
{}

void FirmwareListMenu::onEnter(App* app, bool isForwardNav) {
    if (isForwardNav) {
        selectedIndex_ = 0;
    }
    OtaManager& ota = app->getOtaManager();
    ota.scanSdForFirmware(); // Re-scan the SD card for firmware every time this menu is entered.
    
    displayItems_.clear();
    const auto& firmwares = ota.getAvailableFirmwares();

    for(size_t i = 0; i < firmwares.size(); ++i) {
        displayItems_.push_back({firmwares[i].version, false, (int)i});
    }

    if (firmwares.empty()) {
        displayItems_.push_back({"No firmware found", true, -1});
    }
    displayItems_.push_back({"Back", true, -1});
    marqueeActive_ = false; // Reset marquee state on enter
    marqueeScrollLeft_ = true;
    animation_.resize(displayItems_.size());
    animation_.init();
    animation_.startIntro(selectedIndex_, displayItems_.size());
}

void FirmwareListMenu::onUpdate(App* app) {
    animation_.update();
}

void FirmwareListMenu::onExit(App* app) {
    marqueeActive_ = false; // Ensure marquee is off on exit
}

void FirmwareListMenu::handleInput(App* app, InputEvent event) {
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
        {
            const auto& selected = displayItems_[selectedIndex_];
            if (selected.isBackButton) {
                app->changeMenu(MenuType::BACK);
            } else {
                const auto& firmwares = app->getOtaManager().getAvailableFirmwares();
                if(selected.firmwareIndex >= 0 && (size_t)selected.firmwareIndex < firmwares.size()) {
                    app->getOtaManager().startSdUpdate(firmwares[selected.firmwareIndex]);
                    app->changeMenu(MenuType::OTA_STATUS);
                }
            }
        }
        break;
        case InputEvent::BTN_BACK_PRESS:
            app->changeMenu(MenuType::BACK);
            break;
        default: break;
    }
}

void FirmwareListMenu::scroll(int direction) {
    if (displayItems_.empty()) return;
    selectedIndex_ += direction;
    if (selectedIndex_ < 0) selectedIndex_ = displayItems_.size() - 1;
    else if (selectedIndex_ >= (int)displayItems_.size()) selectedIndex_ = 0;
    
    animation_.setTargets(selectedIndex_, displayItems_.size());
    marqueeActive_ = false;      // <-- Reset Marquee on scroll
    marqueeScrollLeft_ = true;   // <-- Reset Marquee on scroll
}

void FirmwareListMenu::draw(App* app, U8G2& display) {
    const int list_start_y = STATUS_BAR_H + 1;
    const int item_h = 18;

    display.setClipWindow(0, list_start_y, 127, 63);
    display.setFont(u8g2_font_6x10_tf);

    for (size_t i = 0; i < displayItems_.size(); ++i) {
        int item_center_y_rel = (int)animation_.itemOffsetY[i];
        float scale = animation_.itemScale[i];
        if (scale <= 0.01f) continue;
        
        int item_center_y_abs = (list_start_y + (63 - list_start_y) / 2) + item_center_y_rel;
        int item_top_y = item_center_y_abs - item_h / 2;

        if (item_top_y > 63 || item_top_y + item_h < list_start_y) continue;
        
        bool isSelected = (i == selectedIndex_);

        if (isSelected) {
            drawRndBox(display, 2, item_top_y, 124, item_h, 2, true);
            display.setDrawColor(0);
        } else {
            display.setDrawColor(1);
        }

        // --- ICON DRAWING (Corrected) ---
        int icon_x = 6;
        int icon_y = item_top_y + (item_h - IconSize::LARGE_HEIGHT) / 2; // Center 15px icon in 18px height
        IconType icon = displayItems_[i].isBackButton ? IconType::NAV_BACK : IconType::SETTING_SYSTEM;
        drawCustomIcon(display, icon_x, icon_y, icon, IconRenderSize::LARGE); // Use LARGE icon
        
        // --- TEXT DRAWING (With Marquee and Padding) ---
        int text_x = icon_x + IconSize::LARGE_WIDTH + 4; // Position text after the large icon
        int text_y_base = item_center_y_abs + 4;
        const int right_padding = 3;
        int text_available_width = (124 - (text_x - 2)) - right_padding; // <-- MODIFIED: Add right-side padding

        const char* textToDisplay = displayItems_[i].label.c_str();

        if (isSelected) {
            updateMarquee(marqueeActive_, marqueePaused_, marqueeScrollLeft_, 
                          marqueePauseStartTime_, lastMarqueeTime_, marqueeOffset_, 
                          marqueeText_, marqueeTextLenPx_, textToDisplay, text_available_width, display);

            // Set clip window with padding
            int clip_window_right_edge = 2 + 124 - right_padding;
            display.setClipWindow(text_x, item_top_y, clip_window_right_edge, item_top_y + item_h); // <-- MODIFIED
            
            if (marqueeActive_) {
                display.drawStr(text_x + (int)marqueeOffset_, text_y_base, marqueeText_);
            } else {
                display.drawStr(text_x, text_y_base, textToDisplay);
            }
            display.setMaxClipWindow(); // Reset clip window after drawing
        } else {
            char* truncated = truncateText(textToDisplay, text_available_width, display);
            display.drawStr(text_x, text_y_base, truncated);
        }
    }
    display.setDrawColor(1);
    display.setMaxClipWindow();
}