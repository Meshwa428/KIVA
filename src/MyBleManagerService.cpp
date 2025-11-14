#include "MyBleManagerService.h"
#include "App.h"
#include "Logger.h"

MyBleManagerService::MyBleManagerService() : bleManager_() {}

void MyBleManagerService::setup(App* app) {
    bleManager_.setup();
    LOG(LogLevel::INFO, "MyBleManagerService", "BLE Manager Service initialized.");
}

void MyBleManagerService::loop() {}

BleKeyboard* MyBleManagerService::startKeyboard() {
    return bleManager_.startKeyboard();
}

void MyBleManagerService::stopKeyboard() {
    bleManager_.stopKeyboard();
}

BleMouse* MyBleManagerService::startMouse() {
    return bleManager_.startMouse();
}

void MyBleManagerService::stopMouse() {
    bleManager_.stopMouse();
}