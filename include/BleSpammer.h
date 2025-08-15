#ifndef BLE_SPAMMER_H
#define BLE_SPAMMER_H

#include <NimBLEDevice.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "BleManager.h" // Include the new BleManager header

class App;

// Defines the different types of BLE spam attacks
enum class BleSpamMode {
    APPLE_JUICE,
    SOUR_APPLE,
    SAMSUNG,
    GOOGLE,
    MICROSOFT,
    ALL,
};

class BleSpammer {
public:
    BleSpammer();
    void setup(App* app, BleManager* bleManager); // Updated setup
    void start(BleSpamMode mode);
    void stop();
    // loop() is removed, as it's being replaced by a dedicated task
    bool isActive() const;
    const char* getModeString() const;

private:
    void executeSpamPacket();
    NimBLEAdvertisementData getAdvertisementData(BleSpamMode type);
    void generateRandomMac(uint8_t* mac);
    const char* generateRandomName();

    // Task-related functions
    static void spamTaskWrapper(void* param);
    void spamTask();

    App* app_;
    BleManager* bleManager_; // Pointer to the BleManager
    TaskHandle_t spamTaskHandle_; // Handle for the spam task

    volatile bool isActive_; // Make volatile for thread safety
    BleSpamMode currentMode_;
    BleSpamMode currentSubMode_; // For 'ALL' mode cycling
};

#endif // BLE_SPAMMER_H