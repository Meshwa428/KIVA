#include "UsbDriveMenu.h"
#include "App.h"
#include "UI_Utils.h"
#include "Logger.h"
#include "SdCardManager.h"
#include <HIDForge.h> // The main library header for MSC

UsbDriveMenu::UsbDriveMenu() {}

void UsbDriveMenu::onEnter(App* app, bool isForwardNav) {
    LOG(LogLevel::INFO, "USB_DRIVE", "Entering USB Mass Storage Mode.");

    // Stop other services that might be using the SD card
    app->getProbeSniffer().stop();
    app->getEvilTwin().stop();
    app->getMusicPlayer().stop();

    // Initialize the SD card using the HIDForge wrapper
    card_ = std::make_unique<SDCardArduino>(Serial, "/sd", 
        static_cast<gpio_num_t>(-1), 
        static_cast<gpio_num_t>(-1), 
        static_cast<gpio_num_t>(-1), 
        static_cast<gpio_num_t>(Pins::SD_CS_PIN));
    
    if (card_ == nullptr || card_->getSectorCount() == 0) {
        LOG(LogLevel::ERROR, "USB_DRIVE", "SD Card initialization failed via HIDForge.");
        app->showPopUp("Error", "SD Card init failed.", [app](App* app_cb) {
            USB.begin(); // Restore CDC
            app_cb->changeMenu(MenuType::BACK);
        }, "OK", "", false);
        return;
    }

    // Initialize the Mass Storage Class device
    storage_ = std::make_unique<UsbMsc>();
    storage_->begin(card_.get(), "Kiva", "SD Card", "1.0");

    // Start the new composite USB device (MSC)
    USB.begin();
    LOG(LogLevel::INFO, "USB_DRIVE", "USB MSC device started.");
}

void UsbDriveMenu::onExit(App* app) {
    LOG(LogLevel::INFO, "USB_DRIVE", "Exiting USB Mass Storage Mode.");
    
    // Cleanly release the HIDForge objects
    storage_.reset();
    card_.reset();

    // Re-initialize the default USB CDC (Serial) device
    USB.begin();

    // Re-initialize the SD card for firmware use
    LOG(LogLevel::INFO, "USB_DRIVE", "Re-initializing SD card for firmware use.");
    if (!SdCardManager::setup()) {
        LOG(LogLevel::ERROR, "USB_DRIVE", "Failed to re-mount SD Card after USB mode!");
        app->showPopUp("SD Card Error", "Could not re-mount SD Card!", nullptr, "OK", "", true);
    }
    
    // Re-initialize the logger as it depends on the SD card
    Logger::getInstance().setup();
}

void UsbDriveMenu::onUpdate(App* app) {
    // Nothing to do in the loop for this implementation
}

void UsbDriveMenu::handleInput(App* app, InputEvent event) {
    if (event != InputEvent::NONE) {
        // Show a confirmation dialog to ensure the user ejects the drive properly.
        // The callback will navigate back only after the user confirms.
        app->showPopUp("Eject Drive", "Safely eject from PC, then press OK to exit.", 
            [](App* app_cb){ 
                app_cb->changeMenu(MenuType::BACK); 
            }, 
            "OK", "", false
        );
    }
}

void UsbDriveMenu::draw(App* app, U8G2& display) {
    drawCustomIcon(display, display.getDisplayWidth()/2 - 8, 16, IconType::USB);
    display.setFont(u8g2_font_7x13B_tr);
    const char* top_text = "USB Drive Mode";
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(top_text))/2, 45, top_text);
    
    display.setFont(u8g2_font_5x7_tf);
    const char* instruction = "Press any key to exit.";
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(instruction))/2, 60, instruction);
}