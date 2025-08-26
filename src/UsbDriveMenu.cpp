#include "UsbDriveMenu.h"
#include "App.h"
#include "UI_Utils.h"
#include "Logger.h"
#include "SdCardManager.h"
#include "Config.h"

// For XIAO ESP32S3, default SPI pins are:
// MISO = D9 (GPIO5)
// MOSI = D10 (GPIO7)
// SCK  = D8 (GPIO6)
#define SD_CARD_MISO_PIN 8
#define SD_CARD_MOSI_PIN 9
#define SD_CARD_SCK_PIN  7

UsbDriveMenu::UsbDriveMenu() : msc_(nullptr), card_(nullptr) {}

void UsbDriveMenu::onEnter(App* app, bool isForwardNav) {
    LOG(LogLevel::INFO, "USB_DRIVE", "Entering USB Mass Storage Mode.");
    app->getProbeSniffer().stop();
    app->getEvilTwin().stop();

    // Create SDCard object using HIDForge's class and KIVA's pins
    card_ = new SDCardArduino(Serial, "/sd", (gpio_num_t)SD_CARD_MISO_PIN, (gpio_num_t)SD_CARD_MOSI_PIN, (gpio_num_t)SD_CARD_SCK_PIN, (gpio_num_t)Pins::SD_CS_PIN);
    
    if (card_ == nullptr || card_->getSectorCount() == 0) {
        LOG(LogLevel::ERROR, "USB_DRIVE", "SD Card initialization for MSC failed.");
        delete card_; card_ = nullptr;
        app->showPopUp("Error", "SD Card Failed", [app](App* app_cb){
            app_cb->changeMenu(MenuType::BACK);
        }, "OK", "", false);
        return;
    }
    
    LOG(LogLevel::INFO, "USB_DRIVE", "Handing SD card control to USB stack.");
    msc_ = new UsbMsc();
    msc_->begin(card_);
    USB.begin(); // Start the USB stack with the MSC device
}

void UsbDriveMenu::onExit(App* app) {
    LOG(LogLevel::INFO, "USB_DRIVE", "Exiting USB Mass Storage Mode.");
    
    // if (USB.isConnected()) {
    //     USB.end();
    // }
    
    if (msc_) {
        delete msc_;
        msc_ = nullptr;
    }
    if (card_) {
        delete card_; // Calls SD.end() in destructor, flushing caches
        card_ = nullptr;
    }
    
    delay(100);

    LOG(LogLevel::INFO, "USB_DRIVE", "Re-initializing SD card for firmware use.");
    if (!SdCardManager::setup()) {
        LOG(LogLevel::ERROR, "USB_DRIVE", "Failed to re-mount SD Card after USB mode!");
        app->showPopUp("SD Card Error", "Could not re-mount SD Card!", nullptr, "OK", "", true);
    }
    Logger::getInstance().setup();
}

void UsbDriveMenu::onUpdate(App* app) {}

void UsbDriveMenu::handleInput(App* app, InputEvent event) {
    if (event != InputEvent::NONE) {
        app->showPopUp("Confirm Exit", "Safely Eject Drive from PC First!", [app](App* app_cb){
            app_cb->changeMenu(MenuType::BACK);
        }, "EXIT", "Cancel", true);
    }
}

void UsbDriveMenu::draw(App* app, U8G2& display) {
    drawCustomIcon(display, display.getDisplayWidth()/2 - 8, 16, IconType::USB);
    display.setFont(u8g2_font_7x13B_tr);
    const char* top_text = "USB Drive Mode";
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(top_text))/2, 45, top_text);
    display.setFont(u8g2_font_5x7_tf);
    
    const char* instruction1 = "Eject from PC before";
    const char* instruction2 = "pressing any key to exit.";
    
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(instruction1))/2, 56, instruction1);
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(instruction2))/2, 63, instruction2);
}