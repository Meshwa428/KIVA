#include "BadUSB.h"
#include <BleKeyboard.h> // <--- Include the real header here

// --- BleHid Implementation (ESP32 BLE Keyboard Wrapper) ---

BleHid::BleHid(const std::string& deviceName) {
    bleKeyboard_.reset(new BleKeyboard(deviceName, "Kiva", 100));
}

// --- NEWLY DEFINED DESTRUCTOR ---
BleHid::~BleHid() {
    // The empty destructor body is sufficient. Its presence in the .cpp file
    // ensures the compiler knows the full type of BleKeyboard when it
    // automatically generates the code to destroy the unique_ptr member.
}

bool BleHid::begin() {
    if (bleKeyboard_) {
        bleKeyboard_->begin();
        return true;
    }
    return false;
}

// ... (rest of BleHid.cpp is unchanged)
void BleHid::end() {
    // Intentionally empty, as the library's destructor does the work.
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