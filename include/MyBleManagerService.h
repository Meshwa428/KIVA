#pragma once

#include "Service.h"
#include <HIDForge.h>

class App;
class BleMouse; // Forward declare BleMouse

class MyBleManagerService : public Service {
public:
    MyBleManagerService();
    void setup(App* app) override;
    void loop() override;

    BleKeyboard* startKeyboard();
    void stopKeyboard();

    // --- NEW METHODS ---
    BleMouse* startMouse();
    void stopMouse();

private:
    BleManager bleManager_;
};