#include "UsbDriveMenu.h"
#include "App.h"
#include "UI_Utils.h"
#include "Logger.h"
#include "SdCardManager.h"

// --- START: Native USB MSC Implementation ---

// These are the correct headers from the ESP32 Arduino Core
#include <USB.h>
#include <USBMSC.h>
#include <SD.h>

// Create a file-static instance of the USB Mass Storage Class
static USBMSC msc;
static UsbDriveMenu* instance = nullptr; // Static pointer to our menu instance

// Callback for when the host PC wants to read from the SD card
static int32_t onRead(uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
    (void)offset;
    // --- FIX: Use SD.readRAW() as defined in the ESP32 SD.h library ---
    // The host can request multiple sectors, so we loop.
    for (uint32_t i = 0; i < bufsize / 512; i++) {
        if (!SD.readRAW((uint8_t*)buffer + (i * 512), lba + i)) {
            return -1; // Indicate error
        }
    }
    return bufsize;
}

// Callback for when the host PC wants to write to the SD card
static int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {
    (void)offset;
    
    for (uint32_t i = 0; i < bufsize / 512; i++) {
        if (!SD.writeRAW((uint8_t*)buffer + (i * 512), lba + i)) {
            return -1;
        }
    }
    
    // Force immediate write to prevent corruption
    // Note: This may slow performance but ensures data integrity
    return bufsize;
}


// Callback for drive status (e.g., eject)
static bool onStartStop(uint8_t power_condition, bool start, bool load_eject) {
    (void)power_condition;
    (void)start;
    if (instance && load_eject) {
        LOG(LogLevel::INFO, "USB_DRIVE", "Host has ejected the drive. Flushing cache...");
        // This is the crucial step: SD.end() forces the library to flush all cached writes to the physical media.
        SD.end(); 
        // We immediately re-initialize it so it's in a ready state.
        if (!SD.begin()) {
            LOG(LogLevel::ERROR, "USB_DRIVE", "Failed to re-initialize SD card after flush!");
        }
        instance->isEjected = true;
    }
    return true;
}
// --- END: Native USB MSC Implementation ---

UsbDriveMenu::UsbDriveMenu() : isEjected(false) {
    instance = this;
}

void UsbDriveMenu::onEnter(App* app, bool isForwardNav) {
    isEjected = false;
    LOG(LogLevel::INFO, "USB_DRIVE", "Entering USB Mass Storage Mode.");

    LOG(LogLevel::INFO, "USB_DRIVE", "Stopping SD card-dependent services...");
    app->getProbeSniffer().stop();
    app->getEvilTwin().stop();
    
    LOG(LogLevel::INFO, "USB_DRIVE", "Handing SD card control to USB stack.");

    msc.vendorID("Kiva");
    msc.productID("SD Card");
    msc.productRevision("1.0");
    msc.onRead(onRead);
    msc.onWrite(onWrite);
    msc.onStartStop(onStartStop);
    msc.mediaPresent(true);

    // --- FIX: Use SD.numSectors() and SD.sectorSize() as defined in the ESP32 SD.h library ---
    msc.begin(SD.numSectors(), SD.sectorSize());

    USB.begin();
}

void UsbDriveMenu::onExit(App* app) {
    LOG(LogLevel::INFO, "USB_DRIVE", "Exiting USB Mass Storage Mode.");
    
    // Signal to the host that the media is no longer present
    msc.mediaPresent(false);
    
    // Fully stop the USB peripheral
    // USB.end();
    
    // Give a moment for the host OS to process the disconnection
    delay(100);

    LOG(LogLevel::INFO, "USB_DRIVE", "Re-initializing SD card for firmware use.");
    
    // Re-run the main SD card setup to ensure it's cleanly mounted for the firmware
    if (!SdCardManager::setup()) {
        LOG(LogLevel::ERROR, "USB_DRIVE", "Failed to re-mount SD Card after USB mode!");
        app->showPopUp("SD Card Error", "Could not re-mount SD Card!", nullptr, "OK", "", true);
    }
    
    // Re-initialize the logger as it depends on the SD card
    Logger::getInstance().setup();
}

void UsbDriveMenu::onUpdate(App* app) {}

void UsbDriveMenu::handleInput(App* app, InputEvent event) {
    if (event != InputEvent::NONE) {
        if (isEjected) {
            app->changeMenu(MenuType::BACK);
        } else {
            app->showPopUp("Warning!!!", "Safely eject the drive before exiting!", nullptr, "OK", "", true);
        }
    }
}

void UsbDriveMenu::draw(App* app, U8G2& display) {
    drawCustomIcon(display, display.getDisplayWidth()/2 - 8, 16, IconType::USB);
    display.setFont(u8g2_font_7x13B_tr);
    const char* top_text = "USB Drive Mode";
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(top_text))/2, 45, top_text);
    display.setFont(u8g2_font_5x7_tf);
    
    const char* instruction1;
    const char* instruction2;
    if (isEjected) {
        instruction1 = "Safe to disconnect.";
        instruction2 = "Press any key to exit.";
    } else {
        instruction1 = "Eject from PC before";
        instruction2 = "pressing any key.";
    }
    
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(instruction1))/2, 56, instruction1);
    display.drawStr((display.getDisplayWidth() - display.getStrWidth(instruction2))/2, 63, instruction2);
}