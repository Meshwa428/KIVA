#ifndef BLE_SPAMMER_H
#define BLE_SPAMMER_H

#include <NimBLEDevice.h>

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

#include "Service.h"

class BleSpammer : public Service {
public:
    BleSpammer();
    void setup(App* app) override;
    void start(BleSpamMode mode);
    void stop();
    void loop();
    bool isActive() const;
    const char* getModeString() const;

private:
    void executeSpamPacket();
    NimBLEAdvertisementData getAdvertisementData(BleSpamMode type);
    void generateRandomMac(uint8_t* mac);
    const char* generateRandomName();

    App* app_;
    bool isActive_;
    BleSpamMode currentMode_;
    BleSpamMode currentSubMode_; // For 'ALL' mode cycling
    unsigned long lastPacketTime_;
};

#endif // BLE_SPAMMER_H