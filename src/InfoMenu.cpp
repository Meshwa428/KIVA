#include "Event.h"
#include "EventDispatcher.h"
#include "InfoMenu.h"
#include "App.h"
#include "SystemDataProvider.h"
#include "UI_Utils.h"
#include <vector>
#include <string>
#include <Arduino.h>
#include <algorithm>

InfoMenu::InfoMenu() : 
    selectedIndex_(0)
{}

void InfoMenu::onEnter(App* app, bool isForwardNav) {
    EventDispatcher::getInstance().subscribe(EventType::APP_INPUT, this);
    buildInfoItems(app);

    if (isForwardNav) {
        selectedIndex_ = 0;
    }
    animation_.resize(infoItems_.size());
    animation_.init();
    animation_.startIntro(selectedIndex_, infoItems_.size(), 10.f);
}

void InfoMenu::onUpdate(App* app) {
    if (animation_.update()) {
        app->requestRedraw();
    }
    buildInfoItems(app); // Re-fetch data every frame for live updates
}

void InfoMenu::onExit(App* app) {
    EventDispatcher::getInstance().unsubscribe(EventType::APP_INPUT, this);
    infoItems_.clear();
}

void InfoMenu::handleInput(InputEvent event, App* app) {
    switch(event) {
        case InputEvent::ENCODER_CW:
        case InputEvent::BTN_DOWN_PRESS:
            scroll(1);
            break;
        case InputEvent::ENCODER_CCW:
        case InputEvent::BTN_UP_PRESS:
            scroll(-1);
            break;
        case InputEvent::BTN_OK_PRESS:
        case InputEvent::BTN_ENCODER_PRESS: {
            if (selectedIndex_ < infoItems_.size()) {
                const auto& item = infoItems_[selectedIndex_];
                if (item.isToggleable) {
                    app->toggleSecondaryWidget(item.widgetType);
                }
            }
            break;
        }
        case InputEvent::BTN_BACK_PRESS:
            EventDispatcher::getInstance().publish(NavigateBackEvent());
            break;
        default:
            break;
    }
}

void InfoMenu::scroll(int direction) {
    if (infoItems_.empty()) return;
    selectedIndex_ += direction;
    if (selectedIndex_ < 0) {
        selectedIndex_ = infoItems_.size() - 1;
    } else if (selectedIndex_ >= (int)infoItems_.size()) {
        selectedIndex_ = 0;
    }
    animation_.setTargets(selectedIndex_, infoItems_.size(), 10.f);
}

void InfoMenu::buildInfoItems(App* app) {
    infoItems_.clear();
    SystemDataProvider& data = app->getSystemDataProvider();
    char buffer[40];

    // RAM
    const auto& ram = data.getRamUsage();
    std::string ramStr = SystemDataProvider::formatBytes(ram.used) + "/" + SystemDataProvider::formatBytes(ram.total);
    infoItems_.push_back({"RAM", ramStr, true, SecondaryWidgetType::WIDGET_RAM});

    // PSRAM
    if (psramFound()) {
        const auto& psram = data.getPsramUsage();
        std::string psramStr = SystemDataProvider::formatBytes(psram.used) + "/" + SystemDataProvider::formatBytes(psram.total);
        infoItems_.push_back({"PSRAM", psramStr, true, SecondaryWidgetType::WIDGET_PSRAM});
    }

    // SD Card
    const auto& sd = data.getSdCardUsage();
    if (sd.total > 0) {
        std::string sdStr = SystemDataProvider::formatBytes(sd.used) + "/" + SystemDataProvider::formatBytes(sd.total);
        infoItems_.push_back({"SD Card", sdStr, true, SecondaryWidgetType::WIDGET_SD});
    } else {
        infoItems_.push_back({"SD Card", "Not Mounted", false, {}});
    }

    // CPU Freq
    snprintf(buffer, sizeof(buffer), "%d MHz", data.getCpuFrequency());
    infoItems_.push_back({"CPU Freq", buffer, true, SecondaryWidgetType::WIDGET_CPU});

    // Temperature
    snprintf(buffer, sizeof(buffer), "%.1f C", data.getTemperature());
    infoItems_.push_back({"Temp", buffer, true, SecondaryWidgetType::WIDGET_TEMP});

    // Uptime
    unsigned long seconds = millis() / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    snprintf(buffer, sizeof(buffer), "%lu:%02lu:%02lu", hours, minutes % 60, seconds % 60);
    infoItems_.push_back({"Uptime", buffer, false, {}});
}

void InfoMenu::draw(App* app, U8G2& display) {
    const int list_start_y = STATUS_BAR_H + 1;
    const int item_h = 10;
    const int num_visible_items = 5;

    display.setClipWindow(0, list_start_y, 127, 63);
    display.setFont(u8g2_font_5x7_tf);

    for (size_t i = 0; i < infoItems_.size(); ++i) {
        int item_center_y_rel = (int)animation_.itemOffsetY[i];
        int item_center_y_abs = (list_start_y + (63 - list_start_y) / 2) + item_center_y_rel;
        
        if (item_center_y_abs < list_start_y - (item_h / 2) || item_center_y_abs > 63 + (item_h / 2)) {
            continue;
        }

        bool isSelected = (i == selectedIndex_);
        
        if (isSelected) {
            display.setDrawColor(1);
            display.drawGlyph(0, item_center_y_abs + 3, '>');
        }
        
        const auto& item = infoItems_[i];
        const int leftColX = 6;
        const int rightColX = 122;
        int baselineY = item_center_y_abs + 3;

        display.setDrawColor(1);
        
        std::string label = item.label;
        if (item.isToggleable && app->isSecondaryWidgetActive(item.widgetType)) {
            label += " *";
        }

        display.drawStr(leftColX, baselineY, label.c_str());
        display.drawStr(rightColX - display.getStrWidth(item.value.c_str()), baselineY, item.value.c_str());
    }

    display.setDrawColor(1);
    display.setMaxClipWindow();

    if (infoItems_.size() > num_visible_items) {
        int scrollbarH = 63 - list_start_y;
        int thumbH = std::max(5, (scrollbarH * num_visible_items) / (int)infoItems_.size());
        int scrollRange = infoItems_.size() - num_visible_items;
        int thumbY = list_start_y;
        if (scrollRange > 0) {
            thumbY += (scrollbarH - thumbH) * selectedIndex_ / scrollRange;
        }
        display.drawFrame(126, list_start_y, 2, scrollbarH); 
        display.drawBox(126, thumbY, 2, thumbH);
    }
}
