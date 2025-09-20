#include "HardwareManager.h"
#include "EventDispatcher.h" // For publishing events
#include "Event.h"           // For event data structures
#include <Arduino.h>
#include <numeric>
#include <WiFi.h>
#include <esp_wifi.h>
#include "Logger.h"
#include "DebugUtils.h" // For logging MUX channels

// --- Define the static instance pointer ---
HardwareManager* HardwareManager::instance_ = nullptr;

// --- Interrupt Service Routine (ISR) ---
void IRAM_ATTR HardwareManager::handleButtonInterrupt() {
    if (instance_) {
        instance_->buttonInterruptFired_ = true;
    }
}

// --- Constructor ---
HardwareManager::HardwareManager() : 
                                     u8g2_main_(U8G2_R0, U8X8_PIN_NONE),
                                     u8g2_small_(U8G2_R0, U8X8_PIN_NONE),
                                     radio1_(Pins::NRF1_CE_PIN, Pins::NRF1_CSN_PIN, SPI_SPEED_NRF), 
                                     radio2_(Pins::NRF2_CE_PIN, Pins::NRF2_CSN_PIN, SPI_SPEED_NRF), 
                                     currentRfClient_(RfClient::NONE),
                                     currentHostClient_(HostClient::NONE),
                                     bleStackInitialized_(false),
                                     buttonInterruptFired_(false),
                                     encPos_(0), lastEncState_(0), encConsecutiveValid_(0),
                                     pcf0_output_state_(0xFF), laserOn_(false), vibrationOn_(false), amplifierOn_(false),
                                     batteryIndex_(0), batteryInitialized_(false), currentSmoothedVoltage_(4.5f),
                                     isCharging_(false), lastBatteryCheckTime_(0), lastValidRawVoltage_(4.5f),
                                     trendBufferIndex_(0), trendBufferFilled_(false), highPerformanceMode_(false)
{
    // Initialize the static instance pointer
    instance_ = this;

    // Initialize input button states
    for (int i = 0; i < 8; ++i)
    {
        prevDbncHState0_[i] = true; // true = released
        lastDbncT0_[i] = 0;
        prevDbncHState1_[i] = true; // true = released
        lastDbncT1_[i] = 0;
        isBtnHeld1_[i] = false;
        btnHoldStartT1_[i] = 0;
        lastRepeatT1_[i] = 0;
    }
    // Initialize the smoothing buffer
    for (int i = 0; i < Battery::NUM_SAMPLES; ++i)
    {
        batteryReadings_[i] = 0.0f;
    }
    // Initialize the linear regression trend buffer
    for (int i = 0; i < TREND_SAMPLES; ++i)
    {
        voltageTrendBuffer_[i] = 0.0f;
    }
}

void HardwareManager::setup()
{
    setLaser(false);
    setVibration(false);
    setAmplifier(false);

    // Initialize PCF1 for inputs (write all 1s to set pins as inputs)
    selectMux(Pins::MUX_CHANNEL_PCF1_NAV);
    writePCF(Pins::PCF1_ADDR, 0xFF);

    analogReadResolution(12);
    updateBattery(); // Get an initial battery reading

    // Prime the encoder's initial state
    selectMux(Pins::MUX_CHANNEL_PCF0_ENCODER);
    uint8_t pcf0S_init = readPCF(Pins::PCF0_ADDR);
    int encA_val = !(pcf0S_init & (1 << Pins::ENC_A));
    int encB_val = !(pcf0S_init & (1 << Pins::ENC_B));
    lastEncState_ = (encB_val << 1) | encA_val;
    encConsecutiveValid_ = 0;

    // Configure interrupt pin
    pinMode(Pins::BTN_INTERRUPT_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(Pins::BTN_INTERRUPT_PIN), handleButtonInterrupt, FALLING);

    // Startup workaround to clear initial interrupt state
    LOG(LogLevel::INFO, "HW_MANAGER", "Performing initial PCF read to clear startup interrupt.");
    selectMux(Pins::MUX_CHANNEL_PCF0_ENCODER);
    readPCF(Pins::PCF0_ADDR);
    selectMux(Pins::MUX_CHANNEL_PCF1_NAV);
    readPCF(Pins::PCF1_ADDR);
    buttonInterruptFired_ = false; // Explicitly clear flag after workaround

    setupTime_ = millis();
}

void HardwareManager::update()
{
    // Time-based events that must run continuously
    if (millis() - lastBatteryCheckTime_ >= Battery::CHECK_INTERVAL_MS) {
        updateBattery();
    }
    // Button repeats must be checked continuously to handle the "hold" duration.
    processButtonRepeats(); 

    // --- MODIFICATION START: Change 'if' to 'while' ---
    // This is the complete fix. By using a 'while' loop, we ensure that if another
    // interrupt fires while we are processing the first one (e.g., during a slow
    // menu change), we will immediately loop again to process the newest button state.
    // This prevents any interrupts from being lost.
    while (buttonInterruptFired_) {
    // --- MODIFICATION END ---
        LOG(LogLevel::DEBUG, "HW_MANAGER", false, "Interrupt Fired! Reading PCF states.");
        
        // 1. Atomically clear the flag at the START of processing.
        // This is crucial for detecting new interrupts that occur during processing.
        buttonInterruptFired_ = false;

        // 2. Read the state of BOTH modules once.
        selectMux(Pins::MUX_CHANNEL_PCF0_ENCODER);
        uint8_t pcf0State = readPCF(Pins::PCF0_ADDR);

        selectMux(Pins::MUX_CHANNEL_PCF1_NAV);
        uint8_t pcf1State = readPCF(Pins::PCF1_ADDR);

        LOG(LogLevel::DEBUG, "HW_MANAGER", false, "  > PCF0 (Encoder) Raw: 0x%02X", pcf0State);
        LOG(LogLevel::DEBUG, "HW_MANAGER", false, "  > PCF1 (Nav Btns) Raw: 0x%02X", pcf1State);
        
        // 3. Process the captured states.
        processEncoder(pcf0State);
        processButton_PCF0(pcf0State);
        processButtons_PCF1(pcf1State);
        
        // The flag is no longer cleared at the end of the block.
        LOG(LogLevel::DEBUG, "HW_MANAGER", false, "Interrupt handled. Re-checking flag for new events.");
    }
}


HardwareManager::RfLock::RfLock(HardwareManager& manager, bool success) 
    : manager_(manager), valid_(success) {}

HardwareManager::RfLock::~RfLock() {
    if (valid_) {
        manager_.releaseRfControl();
    }
}

void HardwareManager::releaseRfControl() {
    // Serial.printf("[HW-RF] Releasing RF lock from client %d\n", (int)currentRfClient_);
    LOG(LogLevel::INFO, "HW_RF", false, "Releasing RF lock from client %d", (int)currentRfClient_);
    if (currentRfClient_ == RfClient::NRF_JAMMER) {
        if (radio1_.isChipConnected()) radio1_.powerDown();
        if (radio2_.isChipConnected()) radio2_.powerDown();
        SPI.end();

    } else if (currentRfClient_ == RfClient::WIFI) {
        WiFi.mode(WIFI_OFF);

    } else if (currentRfClient_ == RfClient::WIFI_PROMISCUOUS) {
        esp_wifi_set_promiscuous(false);
        WiFi.mode(WIFI_OFF);
    
    } else if (currentRfClient_ == RfClient::ROGUE_AP) {
        WiFi.softAPdisconnect(true);
        WiFi.enableAP(false);
        WiFi.mode(WIFI_OFF);
    }
    currentRfClient_ = RfClient::NONE;
}

std::unique_ptr<HardwareManager::RfLock> HardwareManager::requestRfControl(RfClient client) {
    if (client == currentRfClient_) {
        return std::unique_ptr<RfLock>(new RfLock(*this, true));
    }

    if (currentRfClient_ != RfClient::NONE) {
        Serial.printf("[HW-RF] DENIED request from client %d. Lock held by %d\n", (int)client, (int)currentRfClient_);
        return std::unique_ptr<RfLock>(new RfLock(*this, false)); 
    }

    Serial.printf("[HW-RF] GRANTED request for client %d\n", (int)client);

    if (client == RfClient::NRF_JAMMER) {
        WiFi.mode(WIFI_OFF);
        delay(100);
        
        bool r1_ok = radio1_.begin() && radio1_.isChipConnected();
        bool r2_ok = radio2_.begin() && radio2_.isChipConnected();

        if (r1_ok || r2_ok) {
            currentRfClient_ = RfClient::NRF_JAMMER;
            auto lock = std::unique_ptr<RfLock>(new RfLock(*this, true));
            if (r1_ok) {
                radio1_.powerUp();
                radio1_.setAutoAck(false);
                radio1_.stopListening();
                radio1_.setRetries(0, 0);
                radio1_.setPayloadSize(5);
                radio1_.setAddressWidth(3);
                radio1_.setPALevel(RF24_PA_MAX, true);
                radio1_.setDataRate(RF24_2MBPS);
                radio1_.setCRCLength(RF24_CRC_DISABLED);
                lock->radio1 = &radio1_;
            }
            if (r2_ok) {
                radio2_.powerUp();
                radio2_.setAutoAck(false);
                radio2_.stopListening();
                radio2_.setRetries(0, 0);
                radio2_.setPayloadSize(5);
                radio2_.setAddressWidth(3);
                radio2_.setPALevel(RF24_PA_MAX, true);
                radio2_.setDataRate(RF24_2MBPS);
                radio2_.setCRCLength(RF24_CRC_DISABLED);
                lock->radio2 = &radio2_;
            }
            return lock;
        } else {
             return std::unique_ptr<RfLock>(new RfLock(*this, false));
        }

    } else if (client == RfClient::WIFI) {
        WiFi.mode(WIFI_STA); 
        currentRfClient_ = RfClient::WIFI;
        return std::unique_ptr<RfLock>(new RfLock(*this, true));

    } else if (client == RfClient::WIFI_PROMISCUOUS) {
        WiFi.softAPdisconnect(true);
        WiFi.disconnect(true, true);
        delay(100);
        
        if (WiFi.mode(WIFI_STA)) {
            esp_wifi_set_promiscuous(true);
            currentRfClient_ = RfClient::WIFI_PROMISCUOUS;
            return std::unique_ptr<RfLock>(new RfLock(*this, true));
        } else {
            return std::unique_ptr<RfLock>(new RfLock(*this, false));
        }
    
    } else if (client == RfClient::ROGUE_AP) {
        WiFi.disconnect(true, true);
        delay(100);

        if (WiFi.mode(WIFI_AP_STA)) {
            currentRfClient_ = RfClient::ROGUE_AP;
            return std::unique_ptr<RfLock>(new RfLock(*this, true));
        } else {
            return std::unique_ptr<RfLock>(new RfLock(*this, false));
        }
    }

    return std::unique_ptr<RfLock>(new RfLock(*this, false));
}

bool HardwareManager::requestHostControl(HostClient client) {
    if (client == currentHostClient_) {
        return true;
    }
    if (currentHostClient_ != HostClient::NONE) {
        LOG(LogLevel::ERROR, "HW_MANAGER", "DENIED request from client %d. Lock held by %d", (int)client, (int)currentHostClient_);
        return false; 
    }

    LOG(LogLevel::INFO, "HW_MANAGER", "GRANTING host control to client %d.", (int)client);

    switch(client) {
        case HostClient::USB_HID:
            USB.begin();
            break;
        case HostClient::BLE_HID:
            break;
        case HostClient::NONE:
            break;
    }

    currentHostClient_ = client;
    return true;
}

void HardwareManager::releaseHostControl() {
    LOG(LogLevel::INFO, "HW_MANAGER", "RELEASING host control from client %d.", (int)currentHostClient_);
    
    switch(currentHostClient_) {
        case HostClient::USB_HID:
            break;
        case HostClient::BLE_HID:
            break;
        case HostClient::NONE:
            break;
    }
    
    currentHostClient_ = HostClient::NONE;
}

void HardwareManager::setPerformanceMode(bool highPerf) {
    highPerformanceMode_ = highPerf;
    Serial.printf("[HW-LOG] Performance mode set to: %s\n", highPerf ? "HIGH" : "NORMAL (UI)");
}

void HardwareManager::setMainBrightness(uint8_t contrast) {
    getMainDisplay().setContrast(contrast);
}

void HardwareManager::setAuxBrightness(uint8_t contrast) {
    getSmallDisplay().setContrast(contrast);
}

void HardwareManager::updateBattery()
{
    unsigned long currentTime = millis();
    if (currentTime - lastBatteryCheckTime_ < Battery::CHECK_INTERVAL_MS)
    {
        return;
    }
    lastBatteryCheckTime_ = currentTime;

    float rawVoltage = readRawBatteryVoltage();

    if (!batteryInitialized_)
    {
        for (int i = 0; i < Battery::NUM_SAMPLES; i++)
        {
            batteryReadings_[i] = rawVoltage;
        }
        batteryInitialized_ = true;
        currentSmoothedVoltage_ = rawVoltage;
    }
    else
    {
        batteryReadings_[batteryIndex_] = rawVoltage;
        batteryIndex_ = (batteryIndex_ + 1) % Battery::NUM_SAMPLES;
        float weightedSum = 0, totalWeight = 0;
        for (int i = 0; i < Battery::NUM_SAMPLES; i++)
        {
            float weight = 1.0f + (float)i / Battery::NUM_SAMPLES;
            int idx = (batteryIndex_ - 1 - i + Battery::NUM_SAMPLES) % Battery::NUM_SAMPLES;
            weightedSum += batteryReadings_[idx] * weight;
            totalWeight += weight;
        }
        currentSmoothedVoltage_ = (totalWeight > 0) ? (weightedSum / totalWeight) : rawVoltage;
    }

    voltageTrendBuffer_[trendBufferIndex_] = currentSmoothedVoltage_;
    trendBufferIndex_ = (trendBufferIndex_ + 1) % TREND_SAMPLES;
    if (!trendBufferFilled_ && trendBufferIndex_ == 0)
    {
        trendBufferFilled_ = true;
    }

    const float FULLY_CHARGED_THRESHOLD = 4.25f;

    if (trendBufferFilled_)
    {
        float sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
        for (int i = 0; i < TREND_SAMPLES; ++i)
        {
            float y = voltageTrendBuffer_[i];
            float x = (float)i;
            sum_x += x;
            sum_y += y;
            sum_xy += x * y;
            sum_x2 += x * x;
        }

        float denominator = (TREND_SAMPLES * sum_x2) - (sum_x * sum_x);
        float slope = 0.0f;
        if (abs(denominator) > 0.001f)
        {
            slope = ((TREND_SAMPLES * sum_xy) - (sum_x * sum_y)) / denominator;
        }

        const float chargingSlopeThreshold = 0.0015f;
        const float dischargingSlopeThreshold = -0.001f;

        if (currentSmoothedVoltage_ >= FULLY_CHARGED_THRESHOLD)
        {
            if (slope > chargingSlopeThreshold)
            {
                isCharging_ = true;
            }
            else
            {
                isCharging_ = false;
            }
        }
        else
        {
            if (slope > chargingSlopeThreshold)
            {
                isCharging_ = true;
            }
            else if (slope < dischargingSlopeThreshold)
            {
                isCharging_ = false;
            }
        }
    }
    else
    {
        isCharging_ = (currentSmoothedVoltage_ > FULLY_CHARGED_THRESHOLD);
    }
}

void HardwareManager::setLaser(bool on)
{
    laserOn_ = on;
    if (on)
    {
        pcf0_output_state_ |= (1 << Pins::LASER_PIN_PCF0);
    }
    else
    {
        pcf0_output_state_ &= ~(1 << Pins::LASER_PIN_PCF0);
    }
    selectMux(Pins::MUX_CHANNEL_PCF0_ENCODER);
    writePCF(Pins::PCF0_ADDR, pcf0_output_state_);
}

void HardwareManager::setVibration(bool on)
{
    vibrationOn_ = on;
    if (on)
    {
        pcf0_output_state_ |= (1 << Pins::MOTOR_PIN_PCF0);
    }
    else
    {
        pcf0_output_state_ &= ~(1 << Pins::MOTOR_PIN_PCF0);
    }
    selectMux(Pins::MUX_CHANNEL_PCF0_ENCODER);
    writePCF(Pins::PCF0_ADDR, pcf0_output_state_);
}

void HardwareManager::setAmplifier(bool on) {
    pinMode(Pins::AMPLIFIER_PIN, OUTPUT);
    if (on) {
        // --- CRITICAL FIX ---
        // Before turning the amplifier ON, we MUST release any hold on the pin.
        gpio_hold_dis((gpio_num_t)Pins::AMPLIFIER_PIN);
        // This line is now redundant as AudioOutputPDM will control the pin signal, but it's safe.
        digitalWrite(Pins::AMPLIFIER_PIN, HIGH); 
        amplifierOn_ = true;
    }
    else {
        amplifierOn_ = false;
        // Drive the pin low to ensure silence.
        digitalWrite(Pins::AMPLIFIER_PIN, LOW);
        // Latch the pin in the LOW state to prevent noise after audio stops.
        gpio_hold_en((gpio_num_t)Pins::AMPLIFIER_PIN);
    }
}

float HardwareManager::readRawBatteryVoltage()
{
    int rawValue = analogRead(Pins::ADC_PIN);
    float voltageAtADC = (rawValue / (float)Battery::ADC_RESOLUTION) * Battery::ADC_REF_VOLTAGE;
    float batteryVoltage = voltageAtADC * Battery::VOLTAGE_DIVIDER_RATIO;

    if (batteryVoltage < 2.5f || batteryVoltage > 4.4f)
    {
        return lastValidRawVoltage_;
    }

    lastValidRawVoltage_ = batteryVoltage;
    return batteryVoltage;
}

uint8_t HardwareManager::calculatePercentage(float voltage) const
{
    const float minVoltage = 2.8f;
    const float maxVoltage = 4.2f;
    if (voltage >= maxVoltage)
    {
        return 100;
    }
    if (voltage <= minVoltage)
    {
        return 0;
    }
    float percentage = (voltage - minVoltage) / (maxVoltage - minVoltage) * 100.0f;
    return (uint8_t)percentage;
}

float HardwareManager::getBatteryVoltage() const { return currentSmoothedVoltage_; }
uint8_t HardwareManager::getBatteryPercentage() const { return calculatePercentage(currentSmoothedVoltage_); }
bool HardwareManager::isCharging() const { return isCharging_; }
bool HardwareManager::isLaserOn() const { return laserOn_; }
bool HardwareManager::isVibrationOn() const { return vibrationOn_; }
bool HardwareManager::isAmplifierOn() const { return amplifierOn_; }

U8G2 &HardwareManager::getMainDisplay()
{
    selectMux(Pins::MUX_CHANNEL_MAIN_DISPLAY);
    return u8g2_main_;
}

U8G2 &HardwareManager::getSmallDisplay()
{
    selectMux(Pins::MUX_CHANNEL_SECOND_DISPLAY);
    return u8g2_small_;
}

void HardwareManager::selectMux(uint8_t channel)
{
    static uint8_t lastSelectedChannel = 255;
    if (channel == lastSelectedChannel)
    {
        return;
    }
    Wire.beginTransmission(Pins::MUX_ADDR);
    Wire.write(1 << channel);
    Wire.endTransmission();
    delayMicroseconds(350);
    lastSelectedChannel = channel;
}

void HardwareManager::writePCF(uint8_t pcfAddress, uint8_t data)
{
    Wire.beginTransmission(pcfAddress);
    Wire.write(data);
    Wire.endTransmission();
}

uint8_t HardwareManager::readPCF(uint8_t pcfAddress)
{
    uint8_t val = 0xFF;
    if (Wire.requestFrom(pcfAddress, (uint8_t)1) == 1)
    {
        val = Wire.read();
    }
    return val;
}

void HardwareManager::processEncoder(uint8_t pcf0State)
{
    static const int cwTable[4] = {1, 3, 0, 2};
    static const int ccwTable[4] = {2, 0, 3, 1};
    static const int requiredConsecutive = 2;

    int cA = !(pcf0State & (1 << Pins::ENC_A));
    int cB = !(pcf0State & (1 << Pins::ENC_B));
    int currentState = (cB << 1) | cA;

    if (currentState == lastEncState_)
    {
        return;
    }
    bool validCW = (currentState == cwTable[lastEncState_]);
    bool validCCW = (currentState == ccwTable[lastEncState_]);

    lastEncState_ = currentState;

    if (validCW)
    {
        if (encConsecutiveValid_ < 0) encConsecutiveValid_ = 0;
        encConsecutiveValid_++;
    }
    else if (validCCW)
    {
        if (encConsecutiveValid_ > 0) encConsecutiveValid_ = 0;
        encConsecutiveValid_--;
    }
    else
    {
        encConsecutiveValid_ = 0;
        return;
    }
    if (millis() - setupTime_ < 300)
    {
        return;
    }

    if (encConsecutiveValid_ >= requiredConsecutive)
    {
        EventDispatcher::getInstance().publish(InputEventData(InputEvent::ENCODER_CW));
        encConsecutiveValid_ = 0;
    }
    else if (encConsecutiveValid_ <= -requiredConsecutive)
    {
        EventDispatcher::getInstance().publish(InputEventData(InputEvent::ENCODER_CCW));
        encConsecutiveValid_ = 0;
    }
}

void HardwareManager::processButton_PCF0(uint8_t pcf0State)
{
    unsigned long currentTime = millis();

    if (millis() - setupTime_ < 300) return;

    // This is the bit for the encoder button on PCF0
    int pin = Pins::ENC_BTN; 
    bool rawState = (pcf0State & (1 << pin));

    // Simple debouncing timer check
    if (currentTime - lastDbncT0_[pin] > DEBOUNCE_DELAY_MS) {
        // If the debounced state is different from the last stable state...
        if (rawState != prevDbncHState0_[pin]) {
            // Update the last stable state
            prevDbncHState0_[pin] = rawState;
            // If the new state is PRESSED (LOW, which is false)...
            if (rawState == false) {
                EventDispatcher::getInstance().publish(InputEventData(InputEvent::BTN_ENCODER_PRESS));
            }
        }
    }
}

void HardwareManager::processButtons_PCF1(uint8_t pcf1State)
{
    unsigned long currentTime = millis();

    if (millis() - setupTime_ < 300) return;

    const int pcf1Pins[] = {Pins::NAV_OK, Pins::NAV_BACK, Pins::NAV_A, Pins::NAV_B, Pins::NAV_UP, Pins::NAV_DOWN, Pins::NAV_LEFT, Pins::NAV_RIGHT};

    for (size_t i = 0; i < sizeof(pcf1Pins) / sizeof(pcf1Pins[0]); ++i)
    {
        int pin = pcf1Pins[i];
        bool rawState = (pcf1State & (1 << pin));

        // Simple debouncing timer check
        if (currentTime - lastDbncT1_[pin] > DEBOUNCE_DELAY_MS) {
            // If the debounced state is different from the last stable state...
            if (rawState != prevDbncHState1_[pin]) {
                // Update the last stable state
                prevDbncHState1_[pin] = rawState;

                if (rawState == false) { // PRESSED
                    EventDispatcher::getInstance().publish(InputEventData(mapPcf1PinToEvent(pin)));
                    isBtnHeld1_[pin] = true;
                    btnHoldStartT1_[pin] = currentTime;
                    lastRepeatT1_[pin] = currentTime;
                } else { // RELEASED
                    isBtnHeld1_[pin] = false;
                }
            }
        }
    }
}


void HardwareManager::processButtonRepeats()
{
    unsigned long currentTime = millis();
    const int pcf1Pins[] = {Pins::NAV_UP, Pins::NAV_DOWN, Pins::NAV_LEFT, Pins::NAV_RIGHT};

    for (int pin : pcf1Pins)
    {
        if (isBtnHeld1_[pin])
        {
            if (currentTime - btnHoldStartT1_[pin] > REPEAT_INIT_DELAY_MS &&
                currentTime - lastRepeatT1_[pin] > REPEAT_INTERVAL_MS)
            {
                EventDispatcher::getInstance().publish(InputEventData(mapPcf1PinToEvent(pin)));
                lastRepeatT1_[pin] = currentTime;
            }
        }
    }
}

InputEvent HardwareManager::mapPcf1PinToEvent(int pin)
{
    switch (pin)
    {
    case Pins::NAV_OK:
        return InputEvent::BTN_OK_PRESS;
    case Pins::NAV_BACK:
        return InputEvent::BTN_BACK_PRESS;
    case Pins::NAV_A:
        return InputEvent::BTN_A_PRESS;
    case Pins::NAV_B:
        return InputEvent::BTN_B_PRESS;
    case Pins::NAV_UP:
        return InputEvent::BTN_UP_PRESS;
    case Pins::NAV_DOWN:
        return InputEvent::BTN_DOWN_PRESS;
    case Pins::NAV_LEFT:
        return InputEvent::BTN_LEFT_PRESS;
    case Pins::NAV_RIGHT:
        return InputEvent::BTN_RIGHT_PRESS;
    default:
        return InputEvent::NONE;
    }
}