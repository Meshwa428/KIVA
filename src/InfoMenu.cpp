#include "Event.h"
#include "EventDispatcher.h"
#include "InfoMenu.h"
#include "App.h"
#include "UI_Utils.h"
#include "SdCardManager.h"
#include "esp_chip_info.h"
#include "esp_psram.h"
#include <vector>
#include <string>
#include <Arduino.h> // For temperatureRead()
#include <algorithm> // For std::max

// Helper function for smart byte formatting (compact suffixes)
static std::string formatBytes(uint64_t bytes) {
    char buffer[20];
    const uint64_t KILO = 1024;
    const uint64_t MEGA = KILO * 1024;
    const uint64_t GIGA = MEGA * 1024;

    if (bytes >= (GIGA / 2)) {
        double gb = (double)bytes / GIGA;
        snprintf(buffer, sizeof(buffer), "%.2fG", gb);
    }
    else if (bytes >= MEGA) {
        double mb = (double)bytes / MEGA;
        snprintf(buffer, sizeof(buffer), "%.1fM", mb);
    }
    else {
        uint64_t kb = bytes / KILO;
        snprintf(buffer, sizeof(buffer), "%lluK", kb);
    }
    return std::string(buffer);
}

InfoMenu::InfoMenu() : 
    uptimeItemIndex_(-1), 
    temperatureItemIndex_(-1),
    selectedIndex_(0)
{}

void InfoMenu::onEnter(App* app, bool isForwardNav) {
    EventDispatcher::getInstance().subscribe(EventType::APP_INPUT, this);
    
    infoItems_.clear();
    uptimeItemIndex_ = -1;
    temperatureItemIndex_ = -1;
    char buffer[40];

    // CPU Freq
    snprintf(buffer, sizeof(buffer), "%d MHz", getCpuFrequencyMhz());
    infoItems_.push_back({"CPU Freq", buffer});

    // Temperature
    infoItems_.push_back({"Temperature", "..."});
    temperatureItemIndex_ = infoItems_.size() - 1;

    // RAM (Used / Total)
    uint32_t totalHeap = ESP.getHeapSize();
    uint32_t freeHeap = ESP.getFreeHeap();
    infoItems_.push_back({"RAM", formatBytes(totalHeap - freeHeap) + "/" + formatBytes(totalHeap)});

    // PSRAM (Used / Total)
    if (psramFound()) {
        uint32_t totalPsram = ESP.getPsramSize();
        uint32_t freePsram = ESP.getFreePsram();
        infoItems_.push_back({"PSRAM", formatBytes(totalPsram - freePsram) + "/" + formatBytes(totalPsram)});
    }

    // SD Card (Used / Total)
    if (SdCardManager::isAvailable()) {
        uint64_t totalBytes = SD.cardSize();
        uint64_t usedBytes = SD.usedBytes();
        infoItems_.push_back({"SD Card", formatBytes(usedBytes) + "/" + formatBytes(totalBytes)});
    } else {
        infoItems_.push_back({"SD Card", "Not Mounted"});
    }
    
    // System Uptime (initial value)
    unsigned long seconds = millis() / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    snprintf(buffer, sizeof(buffer), "%lu:%02lu:%02lu", hours, minutes % 60, seconds % 60);
    infoItems_.push_back({"Uptime", buffer});
    uptimeItemIndex_ = infoItems_.size() - 1;

    // Initialize scrolling animation
    if (isForwardNav) {
        selectedIndex_ = 0;
    }
    animation_.resize(infoItems_.size());
    animation_.init();
    // Use the new method to provide a custom item spacing
    animation_.startIntro(selectedIndex_, infoItems_.size(), 10.f);
}

void InfoMenu::onUpdate(App* app) {
    animation_.update();
    
    // Update Uptime
    if (uptimeItemIndex_ != -1 && (size_t)uptimeItemIndex_ < infoItems_.size()) {
        char buffer[40];
        unsigned long seconds = millis() / 1000;
        unsigned long minutes = seconds / 60;
        unsigned long hours = minutes / 60;
        snprintf(buffer, sizeof(buffer), "%lu:%02lu:%02lu", hours, minutes % 60, seconds % 60);
        infoItems_[uptimeItemIndex_].second = buffer;
    }

    // Update Temperature
    if (temperatureItemIndex_ != -1 && (size_t)temperatureItemIndex_ < infoItems_.size()) {
        char buffer[16];
        float temp = temperatureRead();
        snprintf(buffer, sizeof(buffer), "%.1f C", temp);
        infoItems_[temperatureItemIndex_].second = buffer;
    }
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
        case InputEvent::BTN_BACK_PRESS:
        case InputEvent::BTN_OK_PRESS:
        case InputEvent::BTN_ENCODER_PRESS:
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
    // Use the new method to provide a custom item spacing
    animation_.setTargets(selectedIndex_, infoItems_.size(), 10.f);
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
        display.drawStr(leftColX, baselineY, item.first.c_str());
        display.drawStr(rightColX - display.getStrWidth(item.second.c_str()), baselineY, item.second.c_str());
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