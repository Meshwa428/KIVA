#ifndef MPU_MANAGER_H
#define MPU_MANAGER_H

#include "Service.h"

// Struct to hold processed sensor data in physical units
struct MPUData {
    float ax, ay, az; // Accelerometer (in G's)
    float gx, gy, gz; // Gyroscope (in degrees/sec)
};

class MPUManager : public Service {
public:
    MPUManager();
    void setup(App* app) override;
    void loop() override;

    bool begin();
    void stop();
    bool isReady() const;
    MPUData readData();

private:
    // Low-level I2C communication helpers
    bool writeRegister(uint8_t regAddress, uint8_t value);
    uint8_t readRegister(uint8_t regAddress);
    bool testConnection();

    App* app_;
    bool isReady_;
    
    // --- NEW: Hard-coded calibration offsets ---
    float gyroXOffset_, gyroYOffset_, gyroZOffset_;
    float accelXOffset_, accelYOffset_, accelZOffset_;
    
    // Resolution values for converting raw data
    float accelResolution_;
    float gyroResolution_;
};

#endif // MPU_MANAGER_H