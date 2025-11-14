#ifndef AIR_MOUSE_SERVICE_H
#define AIR_MOUSE_SERVICE_H

#include "Service.h"
#include "HIDForge.h"
#include <memory>

class MPUManager;

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

    bool isConnected() const;
    SensorAngles getAngles() const;

    uint32_t getResourceRequirements() const override;

private:
    App* app_;
    MPUManager* mpuManager_;
    std::unique_ptr<MouseInterface> activeMouse_;
    bool isActive_;
    Mode currentMode_;

    unsigned long lastUpdateTime_;
    
    // Variables for the UI visualizer
    float anglePitch_, angleRoll_;
    float pitchAccel_, rollAccel_;

    // --- NEW: Variables for smoothed velocity ---
    float velocityX_, velocityY_;
};

#endif // AIR_MOUSE_SERVICE_H