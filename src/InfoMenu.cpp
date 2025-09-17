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

InfoMenu::InfoMenu() : uptimeItemIndex_(-1) {}

void InfoMenu::onEnter(App* app, bool isForwardNav) {
    EventDispatcher::getInstance().subscribe(EventType::APP_INPUT, this);
    // This function now runs only ONCE when the menu is opened.
    infoItems_.clear();
    uptimeItemIndex_ = -1;
    char buffer[40];

    // CPU Freq
    snprintf(buffer, sizeof(buffer), "%d MHz", getCpuFrequencyMhz());
    infoItems_.push_back({"CPU Freq", buffer});

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
    
    // Store the index of the uptime item so we can update it efficiently.
    uptimeItemIndex_ = infoItems_.size() - 1;
}

void InfoMenu::onUpdate(App* app) {
    // This function is called every frame, but only updates the uptime.
    if (uptimeItemIndex_ != -1 && uptimeItemIndex_ < infoItems_.size()) {
        char buffer[40];
        unsigned long seconds = millis() / 1000;
        unsigned long minutes = seconds / 60;
        unsigned long hours = minutes / 60;
        snprintf(buffer, sizeof(buffer), "%lu:%02lu:%02lu", hours, minutes % 60, seconds % 60);
        
        // Update only the value string for the uptime item.
        infoItems_[uptimeItemIndex_].second = buffer;
    }
}

void InfoMenu::onExit(App* app) {
    EventDispatcher::getInstance().unsubscribe(EventType::APP_INPUT, this);
    // Clear data to free memory
    infoItems_.clear();
    uptimeItemIndex_ = -1;
}

void InfoMenu::handleInput(InputEvent event, App* app) {
    // Any button press will exit the info screen.
    if (event != InputEvent::NONE) {
        EventDispatcher::getInstance().publish(NavigateBackEvent());
    }
}

void InfoMenu::draw(App* app, U8G2& display) {
    // --- Layout Calculation (runs every frame, but is very fast) ---
    display.setFont(u8g2_font_5x7_tf);
    const int lineHeight = 9;
    const int footerHeight = 8;
    const int totalItemHeight = infoItems_.size() * lineHeight;
    const int totalContentHeight = totalItemHeight + footerHeight;
    
    const int drawableAreaTopY = STATUS_BAR_H + 1;
    const int drawableAreaHeight = 63 - drawableAreaTopY;
    
    int contentBlockTopY = drawableAreaTopY + (drawableAreaHeight - totalContentHeight) / 2;
    if (contentBlockTopY < drawableAreaTopY) {
        contentBlockTopY = drawableAreaTopY;
    }

    // --- Drawing (now just renders the stored data) ---
    const int leftColX = 6;
    const int rightColX = 122;
    int currentTopY = contentBlockTopY;

    for (const auto& item : infoItems_) {
        int baselineY = currentTopY + display.getAscent();
        display.drawStr(leftColX, baselineY, item.first.c_str());
        display.drawStr(rightColX - display.getStrWidth(item.second.c_str()), baselineY, item.second.c_str());
        currentTopY += lineHeight;
    }

    // --- Footer ---
    int footerY = 63;
    const char* instruction = "Press any key to exit";
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(instruction)) / 2, footerY, instruction);
}
