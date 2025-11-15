#include "MPUManager.h"
#include "App.h"
#include "HardwareManager.h"
#include "Logger.h"

// MPU-6500 Register Map Constants
const uint8_t MPU_ADDRESS         = 0x68;
const uint8_t MPU_RA_WHO_AM_I     = 0x75;
const uint8_t MPU_RA_PWR_MGMT_1   = 0x6B;
const uint8_t MPU_RA_GYRO_CONFIG  = 0x1B;
const uint8_t MPU_RA_ACCEL_CONFIG = 0x1C;
const uint8_t MPU_RA_ACCEL_XOUT_H = 0x3B;

MPUManager::MPUManager() 
    : app_(nullptr), isReady_(false), 
      gyroXOffset_(0), gyroYOffset_(0), gyroZOffset_(0),
      accelXOffset_(0), accelYOffset_(0), accelZOffset_(0),
      accelResolution_(0.0f), gyroResolution_(0.0f) {}

void MPUManager::setup(App* app) {
    app_ = app;
}

void MPUManager::loop() { /* Not needed */ }

bool MPUManager::begin() {
    if (isReady_) return true;

    // 1. Wake the MPU from sleep mode
    if (!writeRegister(MPU_RA_PWR_MGMT_1, 0x00)) {
        LOG(LogLevel::ERROR, "MPU_MGR", "Failed to wake up MPU.");
        return false;
    }
    delay(100); // Wait for the chip to stabilize

    // 2. Verify the connection with our custom test
    if (!testConnection()) {
        isReady_ = false;
        return false;
    }

    // 3. Configure Gyroscope Full-Scale Range to +/- 500 dps
    if (!writeRegister(MPU_RA_GYRO_CONFIG, 0b00001000)) {
        LOG(LogLevel::ERROR, "MPU_MGR", "Failed to configure Gyro.");
        return false;
    }
    gyroResolution_ = 500.0f / 32768.0f; // LSB sensitivity for +/- 500 dps is 65.5

    // 4. Configure Accelerometer Full-Scale Range to +/- 8g
    if (!writeRegister(MPU_RA_ACCEL_CONFIG, 0b00010000)) {
        LOG(LogLevel::ERROR, "MPU_MGR", "Failed to configure Accelerometer.");
        return false;
    }
    accelResolution_ = 8.0f / 32768.0f; // LSB sensitivity for +/- 8g is 4096
    
    // --- THIS IS THE FIX ---
    // 5. Apply the hard-coded offsets for THIS specific MPU chip.
    // These are the raw integer offsets you provided.
    const int16_t rawGyroOffsetX = 124;
    const int16_t rawGyroOffsetY = -80;
    const int16_t rawGyroOffsetZ = 150;
    const int16_t rawAccelOffsetX = -132;
    const int16_t rawAccelOffsetY = -47;
    const int16_t rawAccelOffsetZ = -86;

    // Convert the raw integer offsets into their floating-point equivalents
    // in physical units (dps and G's) using the resolutions we just configured.
    gyroXOffset_ = (float)rawGyroOffsetX * gyroResolution_;
    gyroYOffset_ = (float)rawGyroOffsetY * gyroResolution_;
    gyroZOffset_ = (float)rawGyroOffsetZ * gyroResolution_;
    accelXOffset_ = (float)rawAccelOffsetX * accelResolution_;
    accelYOffset_ = (float)rawAccelOffsetY * accelResolution_;
    accelZOffset_ = (float)rawAccelOffsetZ * accelResolution_;
    // NOTE: For accelerometers, a proper calibration also accounts for gravity (1G).
    // The Z-axis offset is often used to ensure it reads 1.0 when flat and not 0.0.
    // For an air mouse, primarily using gyro, this direct offset conversion is sufficient.

    LOG(LogLevel::INFO, "MPU_MGR", "MPU Initialized and applied hard-coded offsets.");
    
    isReady_ = true;
    return true;
}

void MPUManager::stop() {
    // Put the MPU back to sleep to save power
    writeRegister(MPU_RA_PWR_MGMT_1, 0b01000000); // Set SLEEP bit
    isReady_ = false;
    LOG(LogLevel::INFO, "MPU_MGR", "MPU service stopped and put to sleep.");
}

bool MPUManager::isReady() const { return isReady_; }

MPUData MPUManager::readData() {
    MPUData data = {0};
    if (!isReady_) return data;

    HardwareManager::I2CMuxLock lock(app_->getHardwareManager(), Pins::MUX_CHANNEL_MPU);
    
    Wire.beginTransmission(MPU_ADDRESS);
    Wire.write(MPU_RA_ACCEL_XOUT_H);
    if (Wire.endTransmission(false) != 0) return data;
    
    if (Wire.requestFrom((uint8_t)MPU_ADDRESS, (uint8_t)14) == 14) {
        int16_t ax_raw = (Wire.read() << 8) | Wire.read();
        int16_t ay_raw = (Wire.read() << 8) | Wire.read();
        int16_t az_raw = (Wire.read() << 8) | Wire.read();
        Wire.read(); Wire.read(); // Skip temperature
        int16_t gx_raw = (Wire.read() << 8) | Wire.read();
        int16_t gy_raw = (Wire.read() << 8) | Wire.read();
        int16_t gz_raw = (Wire.read() << 8) | Wire.read();

        // Convert raw ADC values to physical units
        data.ax = (float)ax_raw * accelResolution_;
        data.ay = (float)ay_raw * accelResolution_;
        data.az = (float)az_raw * accelResolution_;
        data.gx = (float)gx_raw * gyroResolution_;
        data.gy = (float)gy_raw * gyroResolution_;
        data.gz = (float)gz_raw * gyroResolution_;

        // Apply the pre-calculated floating-point offsets to the readings.
        data.ax -= accelXOffset_;
        data.ay -= accelYOffset_;
        data.az -= accelZOffset_;
        data.gx -= gyroXOffset_;
        data.gy -= gyroYOffset_;
        data.gz -= gyroZOffset_;
    }
    return data;
}

bool MPUManager::testConnection() {
    uint8_t whoami = readRegister(MPU_RA_WHO_AM_I);
    LOG(LogLevel::INFO, "MPU_MGR", "WHO_AM_I register returned: 0x%02X", whoami);

    if (whoami == 0x70 || whoami == 0x71 || whoami == 0x73) {
        LOG(LogLevel::INFO, "MPU_MGR", "MPU connection successful.");
        return true;
    } else {
        LOG(LogLevel::ERROR, "MPU_MGR", "MPU connection failed. Incorrect WHO_AM_I value.");
        return false;
    }
}

bool MPUManager::writeRegister(uint8_t regAddress, uint8_t value) {
    HardwareManager::I2CMuxLock lock(app_->getHardwareManager(), Pins::MUX_CHANNEL_MPU);
    Wire.beginTransmission(MPU_ADDRESS);
    Wire.write(regAddress);
    Wire.write(value);
    return (Wire.endTransmission() == 0);
}

uint8_t MPUManager::readRegister(uint8_t regAddress) {
    HardwareManager::I2CMuxLock lock(app_->getHardwareManager(), Pins::MUX_CHANNEL_MPU);
    byte value = 0;
    Wire.beginTransmission(MPU_ADDRESS);
    Wire.write(regAddress);
    if (Wire.endTransmission(false) != 0) return 0;
    
    Wire.requestFrom((uint8_t)MPU_ADDRESS, (uint8_t)1);
    if (Wire.available()) {
        value = Wire.read();
    }
    return value;
}