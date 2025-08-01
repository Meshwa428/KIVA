#include "DuckyScriptRunner.h"
#include <BleKeyboard.h>

// This class now acts as a wrapper around the BleKeyboard object
// managed by the BleManager.

BleHid::BleHid(BleKeyboard* keyboard) : bleKeyboard_(keyboard) {}

BleHid::~BleHid() {
    // Do not delete bleKeyboard_, it is owned and managed by BleManager
}

bool BleHid::begin() {
    // The BleKeyboard object is already "begun" by the BleManager task.
    // This method is just for interface compatibility.
    return bleKeyboard_ != nullptr;
}

void BleHid::end() {
    // The BleKeyboard object will be "ended" by the BleManager task.
    // This method is just for interface compatibility.
}

size_t BleHid::press(uint8_t k) {
    if (bleKeyboard_) {
        return bleKeyboard_->press(k);
    }
    return 0;
}

size_t BleHid::release(uint8_t k) {
    if (bleKeyboard_) {
        return bleKeyboard_->release(k);
    }
    return 0;
}

void BleHid::releaseAll() {
    if (bleKeyboard_) {
        bleKeyboard_->releaseAll();
    }
}

size_t BleHid::write(const uint8_t *buffer, size_t size) {
    if (bleKeyboard_) {
        return bleKeyboard_->write(buffer, size);
    }
    return 0;
}

bool BleHid::isConnected() {
    if (bleKeyboard_) {
        return bleKeyboard_->isConnected();
    }
    return false;
}