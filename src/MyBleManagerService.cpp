#include "MyBleManagerService.h"
#include "App.h"
#include "Logger.h"

MyBleManagerService::MyBleManagerService() : bleManager_() {}

void MyBleManagerService::setup(App* app) {
    // The BleManager setup method doesn't take an App* parameter.
    // It's likely that it initializes the BLE stack or similar.
    bleManager_.setup();
    LOG(LogLevel::INFO, "MyBleManagerService", "BLE Manager Service initialized.");
}

void MyBleManagerService::loop() {
    // BleManager doesn't have a loop method, so nothing to do here.
}

BleKeyboard* MyBleManagerService::startKeyboard() {
    return bleManager_.startKeyboard();
}

void MyBleManagerService::stopKeyboard() {
    bleManager_.stopKeyboard();
}
