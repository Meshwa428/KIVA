#include "Event.h"
#include "EventDispatcher.h"
#include "Event.h"
#include "EventDispatcher.h"
#include "EvilTwinActiveMenu.h"
#include "App.h"
#include "EvilTwin.h" 
#include "UI_Utils.h" 
#include <algorithm> // For std::max

EvilTwinActiveMenu::EvilTwinActiveMenu() :
    topDisplayIndex_(0),
    selectedIndex_(0),
    lastKnownVictimCount_(0)
{}

void EvilTwinActiveMenu::onEnter(App* app, bool isForwardNav) {
    EventDispatcher::getInstance().subscribe(EventType::APP_INPUT, this);
    if (isForwardNav) {
        // Reset list state when the menu is entered
        topDisplayIndex_ = 0;
        selectedIndex_ = 0;
    }
    lastKnownVictimCount_ = 0;
}

void EvilTwinActiveMenu::onExit(App* app) {
    EventDispatcher::getInstance().unsubscribe(EventType::APP_INPUT, this);
    app->getEvilTwin().stop();
}

void EvilTwinActiveMenu::onUpdate(App* app) {
    auto& evilTwin = app->getEvilTwin();
    if (evilTwin.getVictimCount() != lastKnownVictimCount_) {
        lastKnownVictimCount_ = evilTwin.getVictimCount();
        
        // Auto-scroll to show the newest victim
        selectedIndex_ = lastKnownVictimCount_ - 1;
        const int maxVisibleItems = 4;
        if (selectedIndex_ >= maxVisibleItems) {
            topDisplayIndex_ = selectedIndex_ - (maxVisibleItems - 1);
        }
    }
}

void EvilTwinActiveMenu::handleInput(InputEvent event, App* app) {
    if (event == InputEvent::BTN_BACK_PRESS || event == InputEvent::BTN_OK_PRESS) {
        EventDispatcher::getInstance().publish(ReturnToMenuEvent(MenuType::WIFI_ATTACKS_LIST));
        return;
    }

    int listSize = app->getEvilTwin().getVictimCount();
    if (listSize == 0) return;

    const int maxVisibleItems = 4;

    if (event == InputEvent::BTN_UP_PRESS || event == InputEvent::ENCODER_CCW) {
        selectedIndex_--;
        if (selectedIndex_ < 0) selectedIndex_ = listSize - 1;
        if (selectedIndex_ < topDisplayIndex_) topDisplayIndex_ = selectedIndex_;
        if (selectedIndex_ == listSize - 1) topDisplayIndex_ = std::max(0, listSize - maxVisibleItems);
    }
    if (event == InputEvent::BTN_DOWN_PRESS || event == InputEvent::ENCODER_CW) {
        selectedIndex_++;
        if (selectedIndex_ >= listSize) selectedIndex_ = 0;
        if (selectedIndex_ >= topDisplayIndex_ + maxVisibleItems) topDisplayIndex_ = selectedIndex_ - (maxVisibleItems - 1);
        if (selectedIndex_ == 0) topDisplayIndex_ = 0;
    }
}

bool EvilTwinActiveMenu::drawCustomStatusBar(App* app, U8G2& display) {
    auto& evilTwin = app->getEvilTwin();
    display.setFont(u8g2_font_6x10_tf);
    display.setDrawColor(1);

    display.drawStr(2, 8, getTitle());

    char victimStr[16];
    snprintf(victimStr, sizeof(victimStr), "Victims: %d", evilTwin.getVictimCount());
    int textWidth = display.getStrWidth(victimStr);
    display.drawStr(128 - textWidth - 2, 8, victimStr);
    
    display.drawLine(0, STATUS_BAR_H - 1, 127, STATUS_BAR_H - 1);
    return true;
}

void EvilTwinActiveMenu::draw(App* app, U8G2& display) {
    auto& evilTwin = app->getEvilTwin();
    if (!evilTwin.isActive()) {
        const char* msg = "Initializing...";
        display.setFont(u8g2_font_7x13B_tr);
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(msg)) / 2, 38, msg);
        return;
    }

    const int footerY = 63;
    const int listStartY = STATUS_BAR_H + 1;
    const int listEndY = footerY - 8;
    const int listHeight = listEndY - listStartY;

    display.setClipWindow(0, listStartY, 127, listEndY); 

    const auto& victims = evilTwin.getCapturedCredentials();
    if (victims.empty()) {
        display.setFont(u8g2_font_6x10_tf);
        const char* msg = "Waiting for victim...";
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(msg)) / 2, listStartY + (listHeight / 2) + 4, msg);
    } else {
        display.setFont(u8g2_font_5x7_tf);
        const int lineHeight = 9;
        const int maxVisibleItems = 4;
        int listSize = victims.size();

        for (int i = 0; i < maxVisibleItems; ++i) {
            int index = topDisplayIndex_ + i;
            if (index < listSize) {
                bool isSelected = (index == selectedIndex_);
                int yPos = listStartY + (i * lineHeight) + 2;

                if (isSelected) {
                    drawRndBox(display, 1, yPos, 126, lineHeight, 1, true); 
                    display.setDrawColor(0);
                } else {
                    display.setDrawColor(1);
                }
                
                std::string line = victims[index].clientMac;
                if (!victims[index].username.empty()) {
                    line += " -> " + victims[index].username;
                }
                
                char* truncated = truncateText(line.c_str(), 122, display);
                display.drawStr(4, yPos + 7, truncated);
            }
        }
    }

    display.setMaxClipWindow(); 
    display.setDrawColor(1);    

    display.setFont(u8g2_font_5x7_tf);
    const char* instruction = "BACK to Stop";
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(instruction)) / 2, footerY, instruction);
}
