#include "KarmaActiveMenu.h"
#include "App.h"
#include "KarmaAttacker.h"
#include "UI_Utils.h"
#include <algorithm>

KarmaActiveMenu::KarmaActiveMenu() :
    topDisplayIndex_(0),
    selectedIndex_(0),
    lastKnownSsidCount_(0)
{}

void KarmaActiveMenu::onEnter(App* app, bool isForwardNav) {
    if (isForwardNav) {
        topDisplayIndex_ = 0;
        selectedIndex_ = 0;
        lastKnownSsidCount_ = 0;
        displaySsids_.clear();
    }
    app->getKarmaAttacker().startSniffing();
}

void KarmaActiveMenu::onExit(App* app) {
    auto& karma = app->getKarmaAttacker();
    // Stop whichever mode is active
    if (karma.isSniffing()) {
        karma.stopSniffing();
    }
    if (karma.isAttacking()) {
        karma.stopAttack();
    }
}

void KarmaActiveMenu::onUpdate(App* app) {
    auto& karma = app->getKarmaAttacker();
    if (karma.isSniffing()) {
        const auto& sniffedNetworks = karma.getSniffedNetworks();
        if (sniffedNetworks.size() != lastKnownSsidCount_) {
            displaySsids_.clear();
            for(const auto& net : sniffedNetworks) {
                displaySsids_.push_back(net.ssid);
            }
            lastKnownSsidCount_ = displaySsids_.size();
        }
    }
}

void KarmaActiveMenu::handleInput(App* app, InputEvent event) {
    auto& karma = app->getKarmaAttacker();

    if (event == InputEvent::BTN_BACK_PRESS) {
        app->changeMenu(MenuType::BACK);
        return;
    }
    
    if (karma.isAttacking()) {
        // In attack mode, any other button also stops the attack
        if (event != InputEvent::NONE) {
            app->changeMenu(MenuType::BACK);
        }
        return;
    }

    // --- Input handling for SNIFFING mode ---
    int listSize = displaySsids_.size();
    if (listSize == 0) return;

    const int maxVisibleItems = 4;

    switch(event) {
        case InputEvent::BTN_UP_PRESS:
        case InputEvent::ENCODER_CCW:
            selectedIndex_ = (selectedIndex_ - 1 + listSize) % listSize;
            if (selectedIndex_ < topDisplayIndex_) {
                topDisplayIndex_ = selectedIndex_;
            } else if (selectedIndex_ == listSize - 1) {
                topDisplayIndex_ = std::max(0, listSize - maxVisibleItems);
            }
            break;

        case InputEvent::BTN_DOWN_PRESS:
        case InputEvent::ENCODER_CW:
            selectedIndex_ = (selectedIndex_ + 1) % listSize;
            if (selectedIndex_ >= topDisplayIndex_ + maxVisibleItems) {
                topDisplayIndex_ = selectedIndex_ - maxVisibleItems + 1;
            } else if (selectedIndex_ == 0) {
                topDisplayIndex_ = 0;
            }
            break;

        case InputEvent::BTN_OK_PRESS:
        case InputEvent::BTN_ENCODER_PRESS:
            {
                const auto& networks = karma.getSniffedNetworks();
                if (selectedIndex_ < networks.size()) {
                    karma.launchAttack(networks[selectedIndex_]);
                }
            }
            break;
        default:
            break;
    }
}


bool KarmaActiveMenu::drawCustomStatusBar(App* app, U8G2& display) {
    auto& karma = app->getKarmaAttacker();
    display.setFont(u8g2_font_6x10_tf);
    display.setDrawColor(1);

    if (karma.isAttacking()) {
        display.drawStr(2, 8, "Karma: Attacking");
    } else {
        display.drawStr(2, 8, "Karma: Sniffing");
        char countStr[16];
        snprintf(countStr, sizeof(countStr), "Found: %d", displaySsids_.size());
        int textWidth = display.getStrWidth(countStr);
        display.drawStr(128 - textWidth - 2, 8, countStr);
    }
    
    display.drawLine(0, STATUS_BAR_H - 1, 127, STATUS_BAR_H - 1);
    return true;
}

void KarmaActiveMenu::draw(App* app, U8G2& display) {
    auto& karma = app->getKarmaAttacker();
    
    if (karma.isAttacking()) {
        // --- ATTACKING UI ---
        display.setFont(u8g2_font_7x13B_tr);
        display.setDrawColor(1);
        const char* title = "Attacking Target:";
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(title)) / 2, 28, title);

        std::string targetSsid = karma.getCurrentTargetSsid();
        char* truncated = truncateText(targetSsid.c_str(), 124, display);
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(truncated)) / 2, 44, truncated);
        
        display.setFont(u8g2_font_5x7_tf);
        const char* instruction = "Press ANY key to Stop";
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(instruction)) / 2, 63, instruction);

    } else {
        // --- SNIFFING UI ---
        const int footerY = 63;
        const int listStartY = STATUS_BAR_H + 1;
        const int listEndY = footerY - 8;
        const int listHeight = listEndY - listStartY;

        if (displaySsids_.empty()) {
            display.setFont(u8g2_font_6x10_tf);
            const char* msg = "Sniffing for probes...";
            display.drawStr((display.getDisplayWidth() - display.getStrWidth(msg)) / 2, listStartY + (listHeight / 2) + 4, msg);
        } else {
            display.setFont(u8g2_font_5x7_tf);
            const int lineHeight = 9;
            const int maxVisibleItems = 4;
            int listSize = displaySsids_.size();

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
                    
                    char* truncated = truncateText(displaySsids_[index].c_str(), 122, display);
                    display.drawStr(4, yPos + 7, truncated);
                }
            }
        }
        
        display.setDrawColor(1);
        display.setFont(u8g2_font_5x7_tf);
        const char* instruction = "OK to attack / BACK to exit";
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(instruction)) / 2, footerY, instruction);
    }
}