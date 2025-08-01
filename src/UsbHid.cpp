#include "DuckyScriptRunner.h"
#include "USB.h"
#include "USBHIDKeyboard.h" 

UsbHid::UsbHid() {
    usb_keyboard_.reset(new USBHIDKeyboard());
}

UsbHid::~UsbHid() {
    // The unique_ptr will handle destruction
}

bool UsbHid::begin() {
    USB.begin(); // USB stack is simple and can be started here
    usb_keyboard_->begin();
    return true;
}
void UsbHid::end() {
    usb_keyboard_->end();
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