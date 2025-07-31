#include "BadUSB.h"
#include "USB.h"
#include "USBHIDKeyboard.h" // <--- Include the real header here

UsbHid::UsbHid() {
    usb_keyboard_.reset(new USBHIDKeyboard());
}

// --- NEWLY DEFINED DESTRUCTOR ---
UsbHid::~UsbHid() {
    // Empty destructor body is sufficient.
}

bool UsbHid::begin() {
    // USB.begin() is now called by HardwareManager
    usb_keyboard_->begin();
    return true;
}
void UsbHid::end() {
    usb_keyboard_->end();
    // USB.end() is now handled by HardwareManager's releaseHostControl()
}
size_t UsbHid::press(uint8_t k) {
    return usb_keyboard_->press(k);
}
size_t UsbHid::release(uint8_t k) {
    return usb_keyboard_->release(k);
}
void UsbHid::releaseAll() {
    usb_keyboard_->releaseAll();
}
size_t UsbHid::write(const uint8_t *buffer, size_t size) {
    return usb_keyboard_->write(buffer, size);
}
bool UsbHid::isConnected() {
    return USB.VID() != 0;
}