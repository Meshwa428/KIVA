#ifndef MOUSE_JITTER_H
#define MOUSE_JITTER_H

#include <HIDForge.h>

class App;

class MouseJitter {
public:
    enum class Mode { USB, BLE };

    MouseJitter();

    void setup(App* app);
    bool start(Mode mode);
    void stop();
    void loop();

    bool isActive() const;
    bool isConnected() const;
    Mode getMode() const;

private:
    App* app_;
    bool isActive_;
    Mode currentMode_;
    MouseInterface* activeMouse_; // Pointer to the active mouse object

    UsbMouse usbMouse_;
    BleMouse bleMouse_;
};

#endif // MOUSE_JITTER_H