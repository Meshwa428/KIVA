#pragma once

#include "Service.h"
#include <HIDForge.h>

class App;

class MyBleManagerService : public Service {
public:
    MyBleManagerService();
    void setup(App* app) override;
    void loop() override;

    BleKeyboard* startKeyboard();
    void stopKeyboard();

private:
    BleManager bleManager_;
};
