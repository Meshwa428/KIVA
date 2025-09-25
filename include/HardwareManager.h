#ifndef HARDWARE_MANAGER_H
#define HARDWARE_MANAGER_H

#include "Config.h"
#include <Wire.h>
#include <vector>
#include <memory> // For std::unique_ptr
#include <RF24.h>
#undef FEATURE // <-- ADD THIS LINE to resolve the macro conflict with BLE libraries
#include <SPI.h>
#include "USB.h"
#include <NimBLEDevice.h>

// Enum to identify which system is requesting RF control
enum class RfClient {
    NONE,
    NRF_JAMMER,
    WIFI,
    WIFI_PROMISCUOUS,
    ROGUE_AP
};

// --- NEW ENUM FOR HOST PERIPHERALS ---
enum class HostClient {
    NONE,
    USB_HID,
    BLE_HID
};

static const uint32_t SPI_SPEED_NRF = 16000000; // 16 MHz

#include "Service.h"

class HardwareManager : public Service {
public:
    // --- NEW RAII LOCK CLASS (defined inside HardwareManager) ---
    class RfLock {
    public:
        ~RfLock(); // Destructor will release the lock
        bool isValid() const { return valid_; }
        RF24* radio1 = nullptr;
        RF24* radio2 = nullptr;

    private:
        friend class HardwareManager; // Allow HardwareManager to create this
        RfLock(HardwareManager& manager, bool success);
        HardwareManager& manager_;
        bool valid_;
    };

    HardwareManager();
    void setup(App* app) override;
    void update(); 

    // --- NEW: RF Control Method ---
    std::unique_ptr<RfLock> requestRfControl(RfClient client);

    // --- NEW: Host Peripheral Control Methods ---
    bool requestHostControl(HostClient client);
    void releaseHostControl();

    // Display accessors
    U8G2& getMainDisplay();
    U8G2& getSmallDisplay();

    // Input accessor

    // Output Control Methods
    void setLaser(bool on);
    void setVibration(bool on);
    void setAmplifier(bool on);
    bool isLaserOn() const;
    bool isVibrationOn() const;
    bool isAmplifierOn() const;

    void selectMux(uint8_t channel);

    void setPerformanceMode(bool highPerf);
    void setMainBrightness(uint8_t contrast);
    void setAuxBrightness(uint8_t contrast);

    // Battery Status Methods
    float getBatteryVoltage() const;
    uint8_t getBatteryPercentage() const;
    bool isCharging() const;

private:
    void releaseRfControl(); // <-- NEW private helper

    // --- MODIFIED: Input methods now take state as a parameter ---
    void processEncoder(uint8_t pcf0State);
    void processButton_PCF0(uint8_t pcf0State);
    void processButtons_PCF1(uint8_t pcf1State);
    void processButtonRepeats(); 
    InputEvent mapPcf1PinToEvent(int pin);
    
    // I2C methods
    void writePCF(uint8_t pcfAddress, uint8_t data);
    uint8_t readPCF(uint8_t pcfAddress);
    
    // Battery Methods
    void updateBattery();
    uint8_t calculatePercentage(float voltage) const;
    float readRawBatteryVoltage();

    // --- Interrupt Service Routine ---
    static void IRAM_ATTR handleButtonInterrupt();
    static HardwareManager* instance_;

    // Displays
    U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2_main_;
    U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2_small_;
    
    // --- NEW: RF Hardware Objects and State ---
    RF24 radio1_;
    RF24 radio2_;
    RfClient currentRfClient_;

    // --- NEW: Host Peripheral State ---
    HostClient currentHostClient_;
    bool bleStackInitialized_;

    // Input state
    volatile bool buttonInterruptFired_;
    bool prevDbncHState0_[8];
    unsigned long lastDbncT0_[8];
    bool prevDbncHState1_[8];
    unsigned long lastDbncT1_[8];
    int encPos_;
    int lastEncState_;
    int encConsecutiveValid_;
    
    // --- Button Hold/Repeat State ---
    bool isBtnHeld1_[8];
    unsigned long btnHoldStartT1_[8];
    unsigned long lastRepeatT1_[8];

    // --- NEW: Input Grace Period ---
    unsigned long setupTime_;

    bool highPerformanceMode_; 
    
    // Output State
    uint8_t pcf0_output_state_;
    bool laserOn_;
    bool vibrationOn_;
    bool amplifierOn_;
    
    // --- Full Battery State Variables ---
    float batteryReadings_[Battery::NUM_SAMPLES];
    int batteryIndex_;
    bool batteryInitialized_;
    float currentSmoothedVoltage_;
    bool isCharging_;
    unsigned long lastBatteryCheckTime_;
    float lastValidRawVoltage_; 

    // Variables for Linear Regression
    static const int TREND_SAMPLES = 25;
    float voltageTrendBuffer_[TREND_SAMPLES];
    int trendBufferIndex_;
    bool trendBufferFilled_;
};

#endif // HARDWARE_MANAGER_H