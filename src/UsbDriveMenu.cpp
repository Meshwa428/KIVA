#include "Event.h"
#include "EventDispatcher.h"
#include "USBCDC.h"
#include "UsbDriveMenu.h"
#include "App.h"
#include "UI_Utils.h"
#include "Logger.h"
#include "SdCardManager.h"
#include <HIDForge.h> // The main library header for MSC

UsbDriveMenu::UsbDriveMenu() : isEjected(false) {}

void UsbDriveMenu::onEnter(App* app, bool isForwardNav) {
    EventDispatcher::getInstance().subscribe(EventType::APP_INPUT, this);
    isEjected = false; // Reset the flag on entry
    LOG(LogLevel::INFO, "USB_DRIVE", "Entering USB Mass Storage Mode.");

    // app->getProbeSniffer().stop();
    // app->getEvilTwin().stop();
    // app->getMusicPlayer().stop();

    card_ = std::make_unique<SDCardArduino>(Serial, "/sd", static_cast<gpio_num_t>(Pins::SD_CS_PIN));
    
    if (card_ == nullptr || card_->getSectorCount() == 0) {
        LOG(LogLevel::ERROR, "USB_DRIVE", "SD Card initialization failed via HIDForge.");
        app->showPopUp("Error", "SD Card init failed.", [app](App* app_cb) {
            USB.begin();
            EventDispatcher::getInstance().publish(NavigateBackEvent());
        }, "OK", "", false);
        return;
    }

    storage_ = std::make_unique<UsbMsc>();
    
    // --- SET THE EJECT CALLBACK ---
    storage_->onEject([this]() {
        LOG(LogLevel::INFO, "USB_DRIVE", "Eject event received from host.");
        this->isEjected = true;
    });

    storage_->begin(card_.get(), "Kiva", "SD Card", "1.0");

    USB.begin();
    LOG(LogLevel::INFO, "USB_DRIVE", "USB MSC device started.");
}

void UsbDriveMenu::onExit(App* app) {
    EventDispatcher::getInstance().unsubscribe(EventType::APP_INPUT, this);
    LOG(LogLevel::INFO, "USB_DRIVE", "Exiting USB Mass Storage Mode.");
    
    storage_->end();
    storage_.reset();
    card_.reset();

    USB.begin();
    Serial.end();
    Serial.begin(115200);

    LOG(LogLevel::INFO, "USB_DRIVE", "Re-initializing SD card for firmware use.");
    if (!SdCardManager::setup()) {
        LOG(LogLevel::ERROR, "USB_DRIVE", "Failed to re-mount SD Card after USB mode!");
        app->showPopUp("SD Card Error", "Could not re-mount SD Card!", nullptr, "OK", "", true);
    }
    
    Logger::getInstance().setup();
}

void UsbDriveMenu::onUpdate(App* app) {}

void UsbDriveMenu::handleInput(InputEvent event, App* app) {
    if (event != InputEvent::NONE) {
        if (isEjected) {
            app->showPopUp(
                "WARNING!!!",                                   // Title
                "A restart is recommended after using USB mode.", // Message
                [](App* app_cb) {                               // OnConfirm callback
                    LOG(LogLevel::WARN, "USB_DRIVE", "User confirmed restart.");
                    app_cb->getHardwareManager().getMainDisplay().clear(); // Clear display before reboot
                    app_cb->getHardwareManager().getMainDisplay().drawStr(30, 32, "Rebooting...");
                    app_cb->getHardwareManager().getMainDisplay().sendBuffer();
                    delay(500);
                    ESP.restart();
                },
                "Restart",                                      // Confirm button text
                "Cancel",                                     // Cancel button text
                false // Let the popup handle navigation back if cancelled
            );
        } else {
            app->showPopUp("Please Eject First", "Safely eject from PC before exiting.", nullptr, "OK", "", true);
        }
    }
}

void UsbDriveMenu::draw(App* app, U8G2& display) {
    drawCustomIcon(display, display.getDisplayWidth()/2 - 8, 16, IconType::USB);
    display.setFont(u8g2_font_7x13B_tr);
    const char* top_text = "USB Drive Mode";
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(top_text))/2, 45, top_text);
    
    display.setFont(u8g2_font_5x7_tf);
    const char* instruction;
    if (isEjected) {
        instruction = "Ejected! Press any key.";
    } else {
        instruction = "Eject from PC to exit.";
    }
    
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(instruction))/2, 60, instruction);
}
