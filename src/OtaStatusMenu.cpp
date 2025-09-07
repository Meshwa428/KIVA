#include "Event.h"
#include "EventDispatcher.h"
#include "OtaStatusMenu.h"
#include "App.h"
#include "OtaManager.h" // To get status
#include "UI_Utils.h"

OtaStatusMenu::OtaStatusMenu() {
    strcpy(titleBuffer_, "Firmware Update");
}

void OtaStatusMenu::onEnter(App* app, bool isForwardNav) {
    EventDispatcher::getInstance().subscribe(EventType::APP_INPUT, this);
    // The OtaManager should already be in an active state.
    // We just display its current status.
}

void OtaStatusMenu::onUpdate(App* app) {
    // The OtaManager is responsible for the final reboot.
    // We just need to keep this screen active.
}

void OtaStatusMenu::onExit(App* app) {
    EventDispatcher::getInstance().unsubscribe(EventType::APP_INPUT, this);
    // When the user explicitly leaves this screen (e.g. presses back on error),
    // we should stop any active OTA process.
    app->getOtaManager().stop();
}

void OtaStatusMenu::handleInput(InputEvent event, App* app) {
    if (event != InputEvent::BTN_BACK_PRESS) return;

    OtaManager& ota = app->getOtaManager();
    OtaState state = ota.getState();
    const OtaProgress& progress = ota.getProgress();

    // Allow user to go back if:
    // 1. An error has occurred.
    // 2. The process is idle (shouldn't happen, but safe).
    // 3. Web OTA is active but the upload hasn't started yet.
    if (state == OtaState::ERROR || state == OtaState::IDLE || (state == OtaState::WEB_ACTIVE && progress.totalBytes == 0) || state == OtaState::BASIC_ACTIVE) {
        EventDispatcher::getInstance().publish(Event{EventType::NAVIGATE_BACK});
    }
}

const char* OtaStatusMenu::getTitle() const {
    return titleBuffer_;
}

void OtaStatusMenu::draw(App* app, U8G2& display) {
    OtaManager& ota = app->getOtaManager();
    OtaState state = ota.getState();
    const OtaProgress& progress = ota.getProgress();
    String statusMsg = ota.getStatusMessage();

    if (state == OtaState::WEB_ACTIVE) strcpy(titleBuffer_, "Web Update");
    else if (state == OtaState::BASIC_ACTIVE) strcpy(titleBuffer_, "Basic OTA");
    else if (state == OtaState::SD_UPDATING) strcpy(titleBuffer_, "SD Update");
    else if (state == OtaState::SUCCESS) strcpy(titleBuffer_, "Update Complete");
    else if (state == OtaState::ERROR) strcpy(titleBuffer_, "Update Failed");
    else strcpy(titleBuffer_, "Firmware Update");

    int y = STATUS_BAR_H + 18;

    if (state == OtaState::WEB_ACTIVE && progress.totalBytes == 0) {
        display.setFont(u8g2_font_6x12_tf);
        
        String ipLine = "IP: " + ota.getDisplayIpAddress();
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(ipLine.c_str())) / 2, y, ipLine.c_str());
        y += 14;

        String passLine = "Pass: " + statusMsg; // Password is in statusMsg for this state
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(passLine.c_str())) / 2, y, passLine.c_str());
        y += 18;

        display.setFont(u8g2_font_5x7_tf);
        const char* instruction = "Connect & browse to IP";
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(instruction)) / 2, y, instruction);
    
    } else if (state == OtaState::BASIC_ACTIVE) {
        const char* activeText = "OTA Active";
        display.setFont(u8g2_font_7x13B_tr); // A nice bold font
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(activeText)) / 2, y, activeText);
        y += 16;

        display.setFont(u8g2_font_6x12_tf);
        String ipLine = "IP: " + ota.getDisplayIpAddress();
        display.drawStr((display.getDisplayWidth() - display.getStrWidth(ipLine.c_str())) / 2, y, ipLine.c_str());
        y += 14;

        display.drawStr((display.getDisplayWidth() - display.getStrWidth(statusMsg.c_str())) / 2, y, statusMsg.c_str());

    } else {
        
        display.setFont(u8g2_font_6x10_tf);
        if (!statusMsg.isEmpty()) {
            char* truncated = truncateText(statusMsg.c_str(), 124, display);
            display.drawStr((display.getDisplayWidth() - display.getStrWidth(truncated)) / 2, y, truncated);
        }
        y += 16;

        int barX = 14, barW = 100, barH = 9;
        int barY = y;
        display.drawRFrame(barX, barY, barW, barH, 2);
        
        if (state == OtaState::ERROR) {
             const char* errorText = "Error!";
            display.setFont(u8g2_font_7x13B_tr);
            display.drawStr((display.getDisplayWidth() - display.getStrWidth(errorText)) / 2, y-4, errorText);
        } else if (state == OtaState::SUCCESS) {
            display.drawBox(barX + 2, barY + 2, barW - 4, barH - 4);
        }
        else if (progress.totalBytes > 0) {
            int innerBarWidth = barW - 4;
            int fillW = ( (float)progress.receivedBytes / (float)progress.totalBytes ) * innerBarWidth;
            
            if (fillW > 0) {
                 if (fillW > innerBarWidth) fillW = innerBarWidth;
                 display.drawBox(barX + 2, barY + 2, fillW, barH - 4);
            }
        }
    }
}
