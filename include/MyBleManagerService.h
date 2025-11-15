// KIVA/include/MyBleManagerService.h

#pragma once

#include "Service.h"
#include <HIDForge.h> // Includes HIDInterface and MouseInterface

class App;

class MyBleManagerService : public Service {
public:
    MyBleManagerService();
    void setup(App* app) override;
    void loop() override;

    // --- FIX: Return the generic interface pointers ---
    HIDInterface* startKeyboard();
    MouseInterface* startMouse();

    void stopKeyboard();
    void stopMouse();

private:
    BleManager bleManager_;
};