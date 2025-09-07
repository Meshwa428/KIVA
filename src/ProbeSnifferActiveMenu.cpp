#include "Event.h"
#include "EventDispatcher.h"
#include "ProbeSnifferActiveMenu.h"
#include "App.h"
#include "ProbeSniffer.h"
#include "UI_Utils.h"
#include <algorithm> // For std::max / std::min
#include <vector>

ProbeSnifferActiveMenu::ProbeSnifferActiveMenu() : 
    topDisplayIndex_(0), 
    selectedIndex_(0),
    lastKnownSsidCount_(0) 
{}

void ProbeSnifferActiveMenu::onEnter(App* app, bool isForwardNav) {
    EventDispatcher::getInstance().subscribe(EventType::INPUT, this);
    topDisplayIndex_ = 0;
    selectedIndex_ = 0;
    lastKnownSsidCount_ = 0;
    displaySsids_.clear();
    if (!app->getProbeSniffer().start()) {
        app->showPopUp("Error", "Failed to start sniffer.", [app](App* app_cb){
            app_cb->changeMenu(MenuType::BACK);
        }, "OK", "", false);
    }
}

void ProbeSnifferActiveMenu::onExit(App* app) {
    EventDispatcher::getInstance().unsubscribe(EventType::INPUT, this);
    app->getProbeSniffer().stop();
}

void ProbeSnifferActiveMenu::onUpdate(App* app) {
    const auto& sniffedSsids = app->getProbeSniffer().getUniqueSsids();
    if (sniffedSsids.size() != lastKnownSsidCount_) {
        displaySsids_ = sniffedSsids; 
        lastKnownSsidCount_ = displaySsids_.size();
        
        selectedIndex_ = displaySsids_.size() - 1;
        // --- UPDATED LOGIC ---
        const int maxVisibleItems = 3; 
        if (selectedIndex_ >= maxVisibleItems) {
            topDisplayIndex_ = selectedIndex_ - (maxVisibleItems - 1);
        }
    }
}

void ProbeSnifferActiveMenu::handleInput(InputEvent event, App* app) {
    if (event == InputEvent::BTN_BACK_PRESS || event == InputEvent::BTN_OK_PRESS) {
        EventDispatcher::getInstance().publish(Event{EventType::NAVIGATE_BACK});
        return;
    }

    int listSize = displaySsids_.size();
    if (listSize == 0) return;

    // --- UPDATED LOGIC ---
    const int maxVisibleItems = 3;

    if (event == InputEvent::BTN_UP_PRESS || event == InputEvent::ENCODER_CCW) {
        selectedIndex_--;
        if (selectedIndex_ < 0) {
            selectedIndex_ = listSize - 1;
        }
        
        if (selectedIndex_ < topDisplayIndex_) {
            topDisplayIndex_ = selectedIndex_;
        }
        else if (selectedIndex_ == listSize - 1) {
            topDisplayIndex_ = std::max(0, listSize - maxVisibleItems);
        }
    }
    if (event == InputEvent::BTN_DOWN_PRESS || event == InputEvent::ENCODER_CW) {
        selectedIndex_++;
        if (selectedIndex_ >= listSize) {
            selectedIndex_ = 0;
        }

        if (selectedIndex_ >= topDisplayIndex_ + maxVisibleItems) {
            topDisplayIndex_ = selectedIndex_ - (maxVisibleItems - 1);
        }
        else if (selectedIndex_ == 0) {
            topDisplayIndex_ = 0;
        }
    }
}

bool ProbeSnifferActiveMenu::drawCustomStatusBar(App* app, U8G2& display) {
    auto& sniffer = app->getProbeSniffer();

    display.setFont(u8g2_font_6x10_tf);
    display.setDrawColor(1);

    display.drawStr(2, 8, getTitle());

    char pktStr[16];
    snprintf(pktStr, sizeof(pktStr), ": %u", sniffer.getPacketCount());
    int pktStrWidth = display.getStrWidth(pktStr);
    display.drawStr(128 - pktStrWidth - 2, 8, pktStr);
    
    display.drawLine(0, STATUS_BAR_H - 1, 127, STATUS_BAR_H - 1);

    return true; 
}

void ProbeSnifferActiveMenu::draw(App* app, U8G2& display) {
    // --- Zone Definitions ---
    const int footerY = 63;
    const int listStartY = STATUS_BAR_H + 1; // Start list immediately after status bar
    const int listEndY = footerY - 8;
    const int listHeight = listEndY - listStartY;

    // --- Draw List Content ---
    display.setClipWindow(0, listStartY, 127, listEndY); 

    if (displaySsids_.empty()) {
        const char* msg = "Listening for probes...";
        int textAreaX = 4, textAreaY = listStartY, textAreaW = 120, textAreaH = listHeight;
        std::vector<const uint8_t*> fonts = {u8g2_font_6x10_tf};
        drawWrappedText(display, msg, textAreaX, textAreaY, textAreaW, textAreaH, fonts);
    } else {
        display.setFont(u8g2_font_6x10_tf);
        // --- RECALCULATED LAYOUT VALUES ---
        const int lineHeight = 11;     // Increased for better spacing
        const int maxVisibleItems = 3; // Adjusted for new lineHeight
        int listSize = displaySsids_.size();

        // --- Draw Scrollbar ---
        if (listSize > maxVisibleItems) {
            int scrollbarH = listHeight;
            int thumbH = std::max(5, (scrollbarH * maxVisibleItems) / listSize);
            int scrollRange = listSize - maxVisibleItems;
            int thumbY = listStartY;
            if (scrollRange > 0) {
                thumbY += (scrollbarH - thumbH) * topDisplayIndex_ / scrollRange;
            }
            display.drawFrame(125, listStartY, 2, scrollbarH); 
            display.drawBox(125, thumbY, 2, thumbH);
        }

        // --- Draw List Items ---
        for (int i = 0; i < maxVisibleItems; ++i) {
            int index = topDisplayIndex_ + i;
            if (index < listSize) {
                
                bool isSelected = (index == selectedIndex_);
                // REMOVED the extra +2 padding from here
                int yPos = listStartY + (i * lineHeight); 

                if (isSelected) {
                    drawRndBox(display, 1, yPos, 122, lineHeight, 1, true); 
                    display.setDrawColor(0);
                } else {
                    display.setDrawColor(1);
                }

                char* truncated = truncateText(displaySsids_[index].c_str(), 118, display);
                // RECALCULATED baseline for perfect vertical centering in the 11px box
                display.drawStr(4, yPos + 9, truncated);
            }
        }
    }

    display.setMaxClipWindow(); 
    display.setDrawColor(1);    

    // --- Draw Footer ---
    display.setFont(u8g2_font_5x7_tf);
    
    char countStr[16];
    if (displaySsids_.empty()) {
        snprintf(countStr, sizeof(countStr), "0/0");
    } else {
        snprintf(countStr, sizeof(countStr), "%d/%d", selectedIndex_ + 1, (int)displaySsids_.size());
    }
    display.drawStr(2, footerY, countStr);

    const char* instruction = "BACK to Stop/Save";
    display.drawStr(128 - display.getStrWidth(instruction) - 2, footerY, instruction);
}
