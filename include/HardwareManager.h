#ifndef HARDWARE_MANAGER_H
#define HARDWARE_MANAGER_H

#include "Config.h"
#include <Wire.h>
#include <vector>

class HardwareManager {
public:
    HardwareManager();
    void setup();
    void update(); 

    // Display accessors
    U8G2& getMainDisplay();
    U8G2& getSmallDisplay();

    // Input accessor
    InputEvent getNextInputEvent(); 

    // Output Control Methods
    void setLaser(bool on);
    void setVibration(bool on);
    bool isLaserOn() const;
    bool isVibrationOn() const;

    // Battery Status Methods
    float getBatteryVoltage() const;
    uint8_t getBatteryPercentage() const;
    bool isCharging() const;

private:
    // Input methods
    void processEncoder();
    // OLD: void processButtons(uint8_t pcf0State, uint8_t pcf1State);
    void processButton_PCF0(); // NEW
    void processButtons_PCF1(); // NEW
    void processButtonRepeats(); 
    InputEvent mapPcf1PinToEvent(int pin);
    
    // I2C methods
    void selectMux(uint8_t channel);
    void writePCF(uint8_t pcfAddress, uint8_t data);
    uint8_t readPCF(uint8_t pcfAddress);
    
    // Battery Methods
    void updateBattery();
    uint8_t calculatePercentage(float voltage) const;
    float readRawBatteryVoltage();

    // Displays
    U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2_main_;
    U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2_small_;
    
    // Input state
    bool prevDbncHState0_[8];
    unsigned long lastDbncT0_[8];
    bool prevDbncHState1_[8];
    unsigned long lastDbncT1_[8];
    int encPos_;
    int lastEncState_;
    int encConsecutiveValid_;
    std::vector<InputEvent> inputQueue_;
    
    // --- Button Hold/Repeat State ---
    bool isBtnHeld1_[8];
    unsigned long btnHoldStartT1_[8];
    unsigned long lastRepeatT1_[8];

    // --- NEW: Input Grace Period ---
    unsigned long setupTime_;
    
    // Output State
    uint8_t pcf0_output_state_;
    bool laserOn_;
    bool vibrationOn_;
    
    // --- Full Battery State Variables ---
    float batteryReadings_[Battery::NUM_SAMPLES];
    int batteryIndex_;
    bool batteryInitialized_;
    float currentSmoothedVoltage_;
    bool isCharging_;
    unsigned long lastBatteryCheckTime_;
    float lastValidRawVoltage_; // Renamed for clarity

    // Variables for Linear Regression
    static const int TREND_SAMPLES = 20;
    float voltageTrendBuffer_[TREND_SAMPLES];
    int trendBufferIndex_;
    bool trendBufferFilled_;
};

#endif // HARDWARE_MANAGER_H