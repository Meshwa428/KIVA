#include "OtaStatusMenu.h"
#include "App.h"
#include "OtaManager.h" // To get status
#include "UI_Utils.h"

OtaStatusMenu::OtaStatusMenu() {
    strcpy(titleBuffer_, "Firmware Update");
}

void OtaStatusMenu::onEnter(App* app) {
    // The OtaManager should already be in an active state.
    // We just display its current status.
}

void OtaStatusMenu::onUpdate(App* app) {
    // The OtaManager is responsible for the final reboot.
    // We just need to keep this screen active.
}

void OtaStatusMenu::onExit(App* app) {
    // When the user explicitly leaves this screen (e.g. presses back on error),
    // we should stop any active OTA process.
    app->getOtaManager().stop();
}

void OtaStatusMenu::handleInput(App* app, InputEvent event) {
    if (event != InputEvent::BTN_BACK_PRESS) return;

    OtaManager& ota = app->getOtaManager();
    OtaState state = ota.getState();
    const OtaProgress& progress = ota.getProgress();

    // Allow user to go back if:
    // 1. An error has occurred.
    // 2. The process is idle (shouldn't happen, but safe).
    // 3. Web OTA is active but the upload hasn't started yet.
    if (state == OtaState::ERROR || state == OtaState::IDLE || (state == OtaState::WEB_ACTIVE && progress.totalBytes == 0)) {
        app->changeMenu(MenuType::BACK);
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

    // --- Phase 1: Set Title ---
    if (state == OtaState::WEB_ACTIVE) strcpy(titleBuffer_, "Web Update");
    else if (state == OtaState::BASIC_ACTIVE) strcpy(titleBuffer_, "Basic OTA");
    else if (state == OtaState::SD_UPDATING) strcpy(titleBuffer_, "SD Update");
    else if (state == OtaState::SUCCESS) strcpy(titleBuffer_, "Update Complete");
    else if (state == OtaState::ERROR) strcpy(titleBuffer_, "Update Failed");
    else strcpy(titleBuffer_, "Firmware Update");

    // --- Phase 2: Draw Content Based on State ---
    int y = STATUS_BAR_H + 18;

    // --- SPECIAL CASE: Web OTA Initial Screen ---
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

    } else { // --- DEFAULT CASE: All other states and Web OTA in progress ---
        
        display.setFont(u8g2_font_6x10_tf);
        if (!statusMsg.isEmpty()) {
            char* truncated = truncateText(statusMsg.c_str(), 124, display);
            display.drawStr((display.getDisplayWidth() - display.getStrWidth(truncated)) / 2, y, truncated);
        }
        y += 16;

        // --- THE UNCURSED PROGRESS BAR ---
        int barX = 14, barW = 100, barH = 9;
        int barY = y;
        display.drawRFrame(barX, barY, barW, barH, 2);
        
        if (state == OtaState::ERROR) {
             const char* errorText = "Error!";
            display.setFont(u8g2_font_7x13B_tr);
            display.drawStr((display.getDisplayWidth() - display.getStrWidth(errorText)) / 2, y-4, errorText);
        } else if (state == OtaState::SUCCESS) {
            display.drawBox(barX + 2, barY + 2, barW - 4, barH - 4); // Full bar on success
        }
        else if (progress.totalBytes > 0) {
            int innerBarWidth = barW - 4;
            int fillW = ( (float)progress.receivedBytes / (float)progress.totalBytes ) * innerBarWidth;
            
            if (fillW > 0) {
                 if (fillW > innerBarWidth) fillW = innerBarWidth; // Clamp
                 display.drawBox(barX + 2, barY + 2, fillW, barH - 4);
            }
        }
    }
}