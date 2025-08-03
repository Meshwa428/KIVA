#include "DuckyScriptRunner.h"
#include <BleKeyboard.h>

BleHid::BleHid(BleKeyboard* keyboard) : bleKeyboard_(keyboard) {
    // No more reset(), just assign the raw pointer
}

BleHid::~BleHid() {
    // Nothing to do, as we don't own the pointer
}

bool BleHid::begin() {
    // The BleManager handles the lifecycle of the keyboard object now.
    // This begin() is part of the HIDInterface but may not be needed
    // for BLE in this new model. We'll leave it as a no-op for safety.
    return bleKeyboard_ != nullptr;
}

void BleHid::end() {
    // BleManager handles this.
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