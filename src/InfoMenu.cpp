#include "InfoMenu.h"
#include "App.h"
#include <WiFi.h>
#include "UI_Utils.h"
#include "SdCardManager.h"
#include "esp_chip_info.h"
#include "esp_psram.h"

// --- HELPER FUNCTION FOR SMART BYTE FORMATTING ---
static std::string formatBytes(uint64_t bytes) {
    char buffer[20];
    const uint64_t KILO = 1024;
    const uint64_t MEGA = KILO * 1024;
    const uint64_t GIGA = MEGA * 1024;

    // Use GB if size is >= 512 MB
    if (bytes >= (GIGA / 2)) {
        double gb = (double)bytes / GIGA;
        snprintf(buffer, sizeof(buffer), "%.2f GB", gb);
    }
    // Use MB if size is >= 1 MB
    else if (bytes >= MEGA) {
        double mb = (double)bytes / MEGA;
        snprintf(buffer, sizeof(buffer), "%.1f MB", mb);
    }
    // Otherwise, use KB
    else {
        uint64_t kb = bytes / KILO;
        snprintf(buffer, sizeof(buffer), "%llu KB", kb);
    }
    return std::string(buffer);
}


InfoMenu::InfoMenu() : selectedIndex_(0) {}

void InfoMenu::onEnter(App* app, bool isForwardNav) {
    infoItems_.clear();
    selectedIndex_ = 0;

    // Device & System Info
    infoItems_.push_back({"Device Name", "KIVA"});

    // CPU & RAM
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%d MHz", getCpuFrequencyMhz());
    infoItems_.push_back({"CPU Freq", buffer});

    // --- USE THE NEW FORMATTING FUNCTION ---
    infoItems_.push_back({"Free RAM", formatBytes(ESP.getFreeHeap())});
    if (psramFound()) {
        infoItems_.push_back({"Free PSRAM", formatBytes(ESP.getFreePsram())});
    }

    // SD Card Info
    if (SdCardManager::isAvailable()) {
        uint64_t totalBytes = SD.cardSize();
        uint64_t usedBytes = SD.usedBytes();
        std::string sdStr = formatBytes(usedBytes) + "/" + formatBytes(totalBytes);
        infoItems_.push_back({"SD Card", sdStr});
    } else {
        infoItems_.push_back({"SD Card", "Not Mounted"});
    }
    
    // System Uptime
    unsigned long seconds = millis() / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    snprintf(buffer, sizeof(buffer), "%lu:%02lu:%02lu", hours, minutes % 60, seconds % 60);
    infoItems_.push_back({"Uptime", buffer});

    animation_.resize(infoItems_.size());
    animation_.init();
    animation_.startIntro(selectedIndex_, infoItems_.size());
}

void InfoMenu::onUpdate(App* app) {
    animation_.update();
}

void InfoMenu::onExit(App* app) {}

void InfoMenu::handleInput(App* app, InputEvent event) {
    switch (event) {
        case InputEvent::BTN_BACK_PRESS:
        case InputEvent::BTN_OK_PRESS:
            app->changeMenu(MenuType::BACK);
            break;
        case InputEvent::ENCODER_CW:
        case InputEvent::BTN_DOWN_PRESS:
            selectedIndex_ = (selectedIndex_ + 1) % infoItems_.size();
            animation_.setTargets(selectedIndex_, infoItems_.size());
            break;
        case InputEvent::ENCODER_CCW:
        case InputEvent::BTN_UP_PRESS:
            selectedIndex_ = (selectedIndex_ - 1 + infoItems_.size()) % infoItems_.size();
            animation_.setTargets(selectedIndex_, infoItems_.size());
            break;
        default:
            break;
    }
}

void InfoMenu::draw(App* app, U8G2& display) {
    const int list_start_y = STATUS_BAR_H + 1;
    const int item_h = 18;

    display.setClipWindow(0, list_start_y, 127, 63);
    display.setFont(u8g2_font_6x10_tf);

    for (size_t i = 0; i < infoItems_.size(); ++i) {
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
        
        // Draw Label
        display.drawStr(6, item_center_y_abs + 4, infoItems_[i].label.c_str());
        
        // Draw Value
        const char* value_text = infoItems_[i].value.c_str();
        int value_width = display.getStrWidth(value_text);
        display.drawStr(128 - value_width - 6, item_center_y_abs + 4, value_text);
    }
    
    display.setDrawColor(1);
    display.setMaxClipWindow();
}