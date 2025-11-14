#include "AirMouseService.h"
#include "App.h"
#include "MPUManager.h"
#include "Logger.h"
#include <cmath>
#include <algorithm> // Required for std::min and std::max

AirMouseService::AirMouseService() 
    : app_(nullptr), mpuManager_(nullptr), isActive_(false),
      lastUpdateTime_(0), anglePitch_(0), angleRoll_(0), pitchAccel_(0), rollAccel_(0),
      velocityX_(0.0f), velocityY_(0.0f)
{}

void AirMouseService::setup(App* app) {
    app_ = app;
    mpuManager_ = &app->getMPUManager();
}

bool AirMouseService::start(Mode mode) {
    if (isActive_) stop();
    
    if (!mpuManager_->begin()) {
        LOG(LogLevel::ERROR, "AIRMOUSE", "Failed to start MPU.");
        return false;
    }

    currentMode_ = mode;
    if (mode == Mode::BLE) {
        BleMouse* mouse = app_->getBleManager().startMouse();
        if (!mouse) { 
            LOG(LogLevel::ERROR, "AIRMOUSE", "Failed to start BLE mouse HID.");
            mpuManager_->stop(); 
            return false; 
        }
        activeMouse_.reset(mouse);
    } else {
        activeMouse_ = std::make_unique<UsbMouse>();
        activeMouse_->begin();
        USB.begin();
    }

    LOG(LogLevel::INFO, "AIRMOUSE", "Starting Air Mouse in %s mode.", (mode == Mode::USB ? "USB" : "BLE"));
    isActive_ = true;
    lastUpdateTime_ = micros();
    
    anglePitch_ = 0;
    angleRoll_ = 0;
    velocityX_ = 0.0f;
    velocityY_ = 0.0f;

    return true;
}

void AirMouseService::stop() {
    if (!isActive_) return;
    LOG(LogLevel::INFO, "AIRMOUSE", "Stopping Air Mouse.");
    if (currentMode_ == Mode::BLE) app_->getBleManager().stopMouse();
    activeMouse_.reset();
    mpuManager_->stop();
    isActive_ = false;
}

void AirMouseService::loop() {
    if (!isActive_ || !isConnected()) return;

    unsigned long currentTime = micros();
    float dt = (currentTime - lastUpdateTime_) / 1000000.0f;
    lastUpdateTime_ = currentTime;

    MPUData data = mpuManager_->readData();

    // --- TUNING PARAMETERS ---
    const float SENSITIVITY = 2.5f; 
    const float SMOOTHING_FACTOR = 0.15f; 
    const float DEADZONE = 1.5f;

    // --- VELOCITY CALCULATION ---

    // 1. Map raw gyro rotation to target velocities.
    float target_vx = (abs(data.gz) > DEADZONE) ? (data.gz * -SENSITIVITY) : 0.0f;
    float target_vy = (abs(data.gx) > DEADZONE) ? (data.gx * -SENSITIVITY) : 0.0f;

    // 2. Apply the smoothing filter.
    velocityX_ = (SMOOTHING_FACTOR * target_vx) + ((1.0f - SMOOTHING_FACTOR) * velocityX_);
    velocityY_ = (SMOOTHING_FACTOR * target_vy) + ((1.0f - SMOOTHING_FACTOR) * velocityY_);

    // 3. Prepare the final delta values for the HID report.
    int dx = static_cast<int>(velocityX_);
    int dy = static_cast<int>(velocityY_);
    
    // --- CRITICAL FIX: CLAMP THE VALUES TO PREVENT OVERFLOW ---
    const int MOUSE_MOVE_MAX = 127;
    const int MOUSE_MOVE_MIN = -127;
    dx = std::max(MOUSE_MOVE_MIN, std::min(MOUSE_MOVE_MAX, dx));
    dy = std::max(MOUSE_MOVE_MIN, std::min(MOUSE_MOVE_MAX, dy));

    // 4. Send the move command only if there is movement.
    if (dx != 0 || dy != 0) {
        activeMouse_->move(dx, dy);
    }
    
    // --- UI Visualizer Update ---
    rollAccel_ = (atan2(data.ay, data.az) * 180.0) / PI;
    pitchAccel_ = (atan2(-data.ax, sqrt(data.ay * data.ay + data.az * data.az)) * 180.0) / PI;
    angleRoll_ = 0.98 * (angleRoll_ + data.gy * dt) + 0.02 * rollAccel_;
    anglePitch_ = 0.98 * (anglePitch_ + data.gx * dt) + 0.02 * pitchAccel_;
}

void AirMouseService::processClick(bool isPrimary) {
    if (!isActive_ || !activeMouse_) return;
    activeMouse_->click(isPrimary ? MOUSE_LEFT : MOUSE_RIGHT);
}

void AirMouseService::processScroll(int amount) {
    if (!isActive_ || !activeMouse_) return;
    activeMouse_->move(0, 0, amount);
}

bool AirMouseService::isConnected() const { return isActive_ && activeMouse_ && activeMouse_->isConnected(); }

SensorAngles AirMouseService::getAngles() const {
    return {anglePitch_, angleRoll_};
}

bool AirMouseService::isActive() const { return isActive_; }

uint32_t AirMouseService::getResourceRequirements() const {
    return isActive_ ? (uint32_t)ResourceRequirement::HOST_PERIPHERAL : (uint32_t)ResourceRequirement::NONE;
}