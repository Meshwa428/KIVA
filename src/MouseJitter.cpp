#include "MouseJitter.h"
#include "App.h"
#include "Logger.h"

MouseJitter::MouseJitter() 
    : app_(nullptr), isActive_(false), currentMode_(Mode::USB), activeMouse_(nullptr), bleMouse_("Kiva Jitter Mouse") {}

void MouseJitter::setup(App* app) {
    app_ = app;
}

bool MouseJitter::start(Mode mode) {
    if (isActive_) return false;
    
    LOG(LogLevel::INFO, "MOUSE_JITTER", "Starting Mouse Jitter in %s mode.", (mode == Mode::USB ? "USB" : "BLE"));
    currentMode_ = mode;

    if (currentMode_ == Mode::USB) {
        activeMouse_ = &usbMouse_;
        activeMouse_->begin();
        USB.begin();
    } else { // BLE
        activeMouse_ = &bleMouse_;
        // The library's begin() method handles NimBLEDevice::init()
        activeMouse_->begin();
    }
    
    isActive_ = true;
    return true;
}

void MouseJitter::stop() {
    if (!isActive_) return;
    
    LOG(LogLevel::INFO, "MOUSE_JITTER", "Stopping Mouse Jitter.");
    
    if (activeMouse_) {
        // The library's end() method now correctly handles deinit
        activeMouse_->end();
    }

    if (currentMode_ == Mode::USB) {
        // USB.end();
        // TODO: not available, figure out how to disable USB stack
    }
    
    activeMouse_ = nullptr;
    isActive_ = false;
}

void MouseJitter::loop() {
    if (!isActive_ || !activeMouse_ || !activeMouse_->isConnected()) {
        return;
    }

    // Jitter logic
    int8_t dx = (esp_random() % 21) - 10; // Random value between -10 and 10
    int8_t dy = (esp_random() % 21) - 10;
    activeMouse_->move(dx, dy);
    delay(30); // Control the jitter speed
}

bool MouseJitter::isActive() const { return isActive_; }
bool MouseJitter::isConnected() const { return activeMouse_ ? activeMouse_->isConnected() : false; }
MouseJitter::Mode MouseJitter::getMode() const { return currentMode_; }