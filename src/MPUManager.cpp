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
    
    isReady_ = true;
    LOG(LogLevel::INFO, "MPU_MGR", "MPU Initialized and configured successfully via custom driver.");
    
    calibrate();
    return true;
}

void MPUManager::stop() {
    // Put the MPU back to sleep to save power
    writeRegister(MPU_RA_PWR_MGMT_1, 0b01000000); // Set SLEEP bit
    isReady_ = false;
    LOG(LogLevel::INFO, "MPU_MGR", "MPU service stopped and put to sleep.");
}

bool MPUManager::isReady() const { return isReady_; }

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

void MPUManager::calibrate() {
    if (!isReady_) return;
    LOG(LogLevel::INFO, "MPU_MGR", "Calibrating Gyro... Keep device still.");
    // --- REMOVED --- app->showPopUp("Calibrating IMU", "Keep device still...", nullptr, "", "", true);
    
    const int num_samples = 1000;
    float sumX = 0, sumY = 0, sumZ = 0;
    
    for (int i = 0; i < num_samples; ++i) {
        MPUData raw = readData();
        sumX += raw.gx;
        sumY += raw.gy;
        sumZ += raw.gz;
        delay(1);
    }
    
    gyroXOffset_ = sumX / num_samples;
    gyroYOffset_ = sumY / num_samples;
    gyroZOffset_ = sumZ / num_samples;

    // --- REMOVED --- app->showPopUp("Calibration OK", "IMU calibration complete.", nullptr, "OK", "", true);
    LOG(LogLevel::INFO, "MPU_MGR", "Calibration complete. Offsets: Gx=%.2f, Gy=%.2f, Gz=%.2f", gyroXOffset_, gyroYOffset_, gyroZOffset_);
}

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

        data.ax = (float)ax_raw * accelResolution_;
        data.ay = (float)ay_raw * accelResolution_;
        data.az = (float)az_raw * accelResolution_;
        
        // Apply software calibration offsets
        data.gx = ((float)gx_raw * gyroResolution_) - gyroXOffset_;
        data.gy = ((float)gy_raw * gyroResolution_) - gyroYOffset_;
        data.gz = ((float)gz_raw * gyroResolution_) - gyroZOffset_;
    }
    return data;
}

// Low-level I2C helpers
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