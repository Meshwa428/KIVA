// KIVA/include/AirMouseService.h

#ifndef AIR_MOUSE_SERVICE_H
#define AIR_MOUSE_SERVICE_H

#include "Service.h"
#include "HIDForge.h"
#include <memory>

class MPUManager;
class UsbMouse; // Forward declare the concrete USB class we own

struct SensorAngles {
    float pitch;
    float roll;
};

class AirMouseService : public Service {
public:
    enum class Mode { USB, BLE };

    AirMouseService();
    void setup(App* app) override;
    void loop() override;

    bool start(Mode mode);
    void stop();

    bool isActive() const;
    void processClick(bool isPrimary);
    void processScroll(int amount);

    void processPress(uint8_t button);
    void processRelease(uint8_t button);

    bool isConnected() const;
    SensorAngles getAngles() const;

    uint32_t getResourceRequirements() const override;

private:
    App* app_;
    MPUManager* mpuManager_;
    bool isActive_;
    Mode currentMode_;

    std::unique_ptr<UsbMouse> usbMouse_; 
    MouseInterface* activeMouse_; 

    unsigned long lastUpdateTime_;
    
    float anglePitch_, angleRoll_;
    float pitchAccel_, rollAccel_;
    float velocityX_, velocityY_;
};

#endif // AIR_MOUSE_SERVICE_H