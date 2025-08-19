#include "UsbDriveMenu.h"
#include "App.h"
#include "UI_Utils.h"
#include "Logger.h"
#include "SdCardManager.h"
#include "Config.h" // For pins

// Destructor to clean up the SDCard object
UsbDriveMenu::~UsbDriveMenu() {
    if (card_) {
        delete card_;
        card_ = nullptr;
    }
}

UsbDriveMenu::UsbDriveMenu() : card_(nullptr) {}

void UsbDriveMenu::onEnter(App* app, bool isForwardNav) {
    LOG(LogLevel::INFO, "USB_DRIVE", "Entering USB Mass Storage Mode.");
    app->getHardwareManager().setPerformanceMode(true); // Ensure responsiveness

    // Stop services that might be using the SD card
    app->getProbeSniffer().stop();
    app->getEvilTwin().stop();
    
    // Create an SDCard object for HIDForge
    card_ = new SDCardArduino(Serial, "/sd", (gpio_num_t)8, (gpio_num_t)9, (gpio_num_t)7, (gpio_num_t)Pins::SD_CS_PIN);
    
    if (card_->getSectorCount() == 0) {
        LOG(LogLevel::ERROR, "USB_DRIVE", "SD Card failed to init for MSC.");
        app->showPopUp("SD Error", "Could not init SD card for USB mode.", [app](App* app_cb){
            app_cb->changeMenu(MenuType::BACK);
        }, "OK", "", false);
        delete card_;
        card_ = nullptr;
        return;
    }

    // Begin the MSC component
    app->getUsbMsc().begin(card_);
    
    // Start the global USB stack
    USB.begin();
}

void UsbDriveMenu::onExit(App* app) {
    LOG(LogLevel::INFO, "USB_DRIVE", "Exiting USB Mass Storage Mode.");
    
    // Stop the global USB stack
    // USB.end();
    
    // Clean up the SDCard object
    if (card_) {
        delete card_;
        card_ = nullptr;
    }
    
    delay(100);

    // Re-initialize SD card and Logger for the main app
    if (!SdCardManager::setup()) {
        LOG(LogLevel::ERROR, "USB_DRIVE", "Failed to re-mount SD Card!");
        app->showPopUp("SD Error", "Could not re-mount SD Card!", nullptr, "OK", "", true);
    }
    Logger::getInstance().setup();
    app->getHardwareManager().setPerformanceMode(false);
}

void UsbDriveMenu::onUpdate(App* app) {}

void UsbDriveMenu::handleInput(App* app, InputEvent event) {
    // Unlike the old version, HIDForge does not provide an eject callback.
    // The user must be trusted to eject from their OS.
    if (event != InputEvent::NONE) {
        app->showPopUp("Exiting", "Safely eject drive from your PC first!", [app](App* app_cb){
            app_cb->changeMenu(MenuType::BACK);
        }, "OK, I did", "Cancel", false);
    }
}

void UsbDriveMenu::draw(App* app, U8G2& display) {
    drawCustomIcon(display, display.getDisplayWidth()/2 - 8, 16, IconType::USB);
    display.setFont(u8g2_font_7x13B_tr);
    const char* top_text = "USB Drive Mode";
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(top_text))/2, 45, top_text);
    display.setFont(u8g2_font_5x7_tf);
    
    const char* instruction1 = "Eject from PC before";
    const char* instruction2 = "pressing any key.";
    
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(instruction1))/2, 56, instruction1);
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(instruction2))/2, 63, instruction2);
}