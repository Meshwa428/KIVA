#include "MainMenu.h"
#include "App.h"
#include "UI_Utils.h"
#include <algorithm> // For std::max

MainMenu::MainMenu() : selectedIndex_(0) {
    menuItems_.push_back({"Tools",     IconType::TOOLS,    MenuType::TOOLS_CAROUSEL});
    menuItems_.push_back({"Games",     IconType::GAMES,    MenuType::GAMES_CAROUSEL});
    menuItems_.push_back({"Settings",  IconType::SETTINGS, MenuType::SETTINGS_CAROUSEL});
    menuItems_.push_back({"Utilities", IconType::UTILITIES_CATEGORY, MenuType::UTILITIES_CAROUSEL});
    menuItems_.push_back({"Info",      IconType::INFO,     MenuType::NONE});
}

void MainMenu::onEnter(App* app) {
    animation_.init();
    animation_.startIntro(selectedIndex_, menuItems_.size());
}

void MainMenu::onUpdate(App* app) {
    animation_.update();
}

void MainMenu::onExit(App* app) {
    // Nothing to clean up
}

void MainMenu::handleInput(App* app, InputEvent event) {
    // Log that this specific menu is handling the input
    Serial.printf("[MainMenu-LOG] handleInput received event: %d\n", static_cast<int>(event));

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
            app->changeMenu(menuItems_[selectedIndex_].targetMenu);
            break;
        case InputEvent::BTN_BACK_PRESS:
            // No action
            break;
        default:
            break;
    }
}

void MainMenu::scroll(int direction) {
    if (menuItems_.empty()) return;
    selectedIndex_ += direction;
    if (selectedIndex_ < 0) {
        selectedIndex_ = menuItems_.size() - 1;
    } else if (selectedIndex_ >= menuItems_.size()) {
        selectedIndex_ = 0;
    }
    animation_.setTargets(selectedIndex_, menuItems_.size());
}

void MainMenu::draw(App* app, U8G2& display) { // Signature updated
    const int icon_h = IconSize::LARGE_HEIGHT;
    const int icon_w = IconSize::LARGE_WIDTH;
    const int icon_padding_right = 4;
    const int list_start_y = STATUS_BAR_H;
    const int list_visible_h = 64 - list_start_y;
    const int list_center_y_abs = list_start_y + list_visible_h / 2;

    display.setClipWindow(0, list_start_y, 127, 63);

    for (size_t i = 0; i < menuItems_.size(); i++) {
        int item_center_y_relative = (int)animation_.itemOffsetY[i];
        float current_scale = animation_.itemScale[i];
        if (current_scale <= 0.01f) continue;

        int text_box_base_width = 65;
        int text_box_base_height = 12;
        int text_box_w_scaled = (int)(text_box_base_width * current_scale);
        int text_box_h_scaled = (int)(text_box_base_height * current_scale);

        int item_abs_center_y = list_center_y_abs + item_center_y_relative;
        int effective_item_h = std::max(icon_h, text_box_h_scaled);
        if (item_abs_center_y - effective_item_h / 2 > 63 || item_abs_center_y + effective_item_h / 2 < list_start_y) continue;

        int total_visual_width = icon_w + icon_padding_right + text_box_w_scaled;
        int item_start_x_abs = (128 - total_visual_width) / 2;
        int icon_x_pos = item_start_x_abs;
        int icon_y_pos = item_abs_center_y - icon_h / 2;
        int text_box_x_pos = icon_x_pos + icon_w + icon_padding_right;
        int text_box_y_pos = item_abs_center_y - text_box_h_scaled / 2;
        bool is_selected_item = (i == selectedIndex_);

        display.setDrawColor(1);
        drawCustomIcon(display, icon_x_pos, icon_y_pos, menuItems_[i].icon);

        display.setFont(u8g2_font_6x10_tf);
        int text_width_pixels = display.getStrWidth(menuItems_[i].label);
        int text_x_render_pos = text_box_x_pos + (text_box_w_scaled - text_width_pixels) / 2;
        int text_y_baseline_render_pos = item_abs_center_y + 4;

        if (text_box_w_scaled > 0 && text_box_h_scaled > 0) {
            if (is_selected_item) {
                display.setDrawColor(1); 
                drawRndBox(display, text_box_x_pos, text_box_y_pos, text_box_w_scaled, text_box_h_scaled, 2, true);
                display.setDrawColor(0); 
                display.drawStr(text_x_render_pos, text_y_baseline_render_pos, menuItems_[i].label);
            } else {
                display.setDrawColor(1); 
                drawRndBox(display, text_box_x_pos, text_box_y_pos, text_box_w_scaled, text_box_h_scaled, 2, false);
                display.drawStr(text_x_render_pos, text_y_baseline_render_pos, menuItems_[i].label);
            }
        }
    }
    display.setDrawColor(1);
    display.setMaxClipWindow();
}