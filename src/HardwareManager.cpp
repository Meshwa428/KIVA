#include "HardwareManager.h"
#include <Arduino.h>
#include <numeric> // Required for std::accumulate

HardwareManager::HardwareManager() : u8g2_main_(U8G2_R0, U8X8_PIN_NONE),
                                     u8g2_small_(U8G2_R0, U8X8_PIN_NONE),
                                     encPos_(0), lastEncState_(0), encConsecutiveValid_(0),
                                     pcf0_output_state_(0xFF), laserOn_(false), vibrationOn_(false),
                                     batteryIndex_(0), batteryInitialized_(false), currentSmoothedVoltage_(4.5f),
                                     isCharging_(false), lastBatteryCheckTime_(0), lastValidRawVoltage_(4.5f),
                                     trendBufferIndex_(0), trendBufferFilled_(false)
{
    // Initialize input button states
    for (int i = 0; i < 8; ++i)
    {
        prevDbncHState0_[i] = true;
        lastDbncT0_[i] = 0;
        prevDbncHState1_[i] = true;
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
    // Note: Wire.begin() and display.begin() are now called from App::setup()
    // to allow the boot screen to show up before this method is called.
    
    // Correctly initialize PCF0 outputs (laser/vibration motor off)
    pcf0_output_state_ = 0xFF;
    pcf0_output_state_ &= ~((1 << Pins::LASER_PIN_PCF0) | (1 << Pins::MOTOR_PIN_PCF0));
    selectMux(Pins::MUX_CHANNEL_PCF0_ENCODER);
    writePCF(Pins::PCF0_ADDR, pcf0_output_state_);

    // Initialize PCF1 for inputs
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

    setupTime_ = millis();
}

void HardwareManager::update()
{
    if (millis() - lastBatteryCheckTime_ >= Battery::CHECK_INTERVAL_MS)
    {
        updateBattery();
    }

    processEncoder();
    processButton_PCF0();  // A new, dedicated function for PCF0 buttons
    processButtons_PCF1(); // A new, dedicated function for PCF1 buttons

    // Finally, handle repeats which don't depend on the MUX
    processButtonRepeats();
}

void HardwareManager::updateBattery() {
    unsigned long currentTime = millis();
    if (currentTime - lastBatteryCheckTime_ < Battery::CHECK_INTERVAL_MS) {
        return;
    }
    lastBatteryCheckTime_ = currentTime;

    // 1. Get a reliable raw voltage reading
    float rawVoltage = readRawBatteryVoltage();

    // 2. Perform weighted moving average smoothing for a stable display value
    if (!batteryInitialized_) {
        for (int i = 0; i < Battery::NUM_SAMPLES; i++) { batteryReadings_[i] = rawVoltage; }
        batteryInitialized_ = true;
        currentSmoothedVoltage_ = rawVoltage;
    } else {
        batteryReadings_[batteryIndex_] = rawVoltage;
        batteryIndex_ = (batteryIndex_ + 1) % Battery::NUM_SAMPLES;
        float weightedSum = 0, totalWeight = 0;
        for (int i = 0; i < Battery::NUM_SAMPLES; i++) {
            float weight = 1.0f + (float)i / Battery::NUM_SAMPLES;
            int idx = (batteryIndex_ - 1 - i + Battery::NUM_SAMPLES) % Battery::NUM_SAMPLES;
            weightedSum += batteryReadings_[idx] * weight;
            totalWeight += weight;
        }
        currentSmoothedVoltage_ = (totalWeight > 0) ? (weightedSum / totalWeight) : rawVoltage;
    }

    // 3. Update the trend buffer with the NEWLY SMOOTHED voltage
    voltageTrendBuffer_[trendBufferIndex_] = currentSmoothedVoltage_;
    trendBufferIndex_ = (trendBufferIndex_ + 1) % TREND_SAMPLES;
    if (!trendBufferFilled_ && trendBufferIndex_ == 0) {
        trendBufferFilled_ = true;
    }

    // 4. --- NEW HYBRID CHARGING DETECTION LOGIC ---
    const float FULLY_CHARGED_THRESHOLD = 4.25f; // A voltage at which charging current typically drops to near zero.
    
    if (trendBufferFilled_) {
        // We have enough data for trend analysis.
        
        // Calculate the slope of the voltage trend.
        float sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
        for (int i = 0; i < TREND_SAMPLES; ++i) {
            float y = voltageTrendBuffer_[i];
            float x = (float)i;
            sum_x += x; sum_y += y; sum_xy += x * y; sum_x2 += x * x;
        }
        
        float denominator = (TREND_SAMPLES * sum_x2) - (sum_x * sum_x);
        float slope = 0.0f;
        if (abs(denominator) > 0.001f) {
            slope = ((TREND_SAMPLES * sum_xy) - (sum_x * sum_y)) / denominator;
        }

        const float chargingSlopeThreshold = 0.0015f;  // A rise of 1.5mV/sec indicates charging
        const float dischargingSlopeThreshold = -0.001f; // A drop of 1mV/sec indicates discharging

        // Now, apply the hybrid logic
        if (currentSmoothedVoltage_ >= FULLY_CHARGED_THRESHOLD) {
            // If voltage is at or above the 'full' threshold, the charging indicator should
            // only be on if the voltage is STILL actively rising. Otherwise, it's full and idle.
            if (slope > chargingSlopeThreshold) {
                isCharging_ = true; // Still in the final constant-voltage charging phase
            } else {
                isCharging_ = false; // Battery is full and stable, charger has stopped.
            }
        } else {
            // If we are below the 'full' threshold, we rely purely on the slope.
            if (slope > chargingSlopeThreshold) {
                isCharging_ = true; // Actively charging
            } else if (slope < dischargingSlopeThreshold) {
                isCharging_ = false; // Actively discharging
            }
            // If in the dead-zone here, state is maintained. This is correct for an idle, partially-charged battery.
        }

    } else {
        // Fallback before we have enough data for regression: use a simple voltage threshold.
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

float HardwareManager::readRawBatteryVoltage()
{
    int rawValue = analogRead(Pins::ADC_PIN);
    float voltageAtADC = (rawValue / (float)Battery::ADC_RESOLUTION) * Battery::ADC_REF_VOLTAGE;
    // Uses the new, calibrated ratio from Config.h
    float batteryVoltage = voltageAtADC * Battery::VOLTAGE_DIVIDER_RATIO;

    if (batteryVoltage < 2.5f || batteryVoltage > 4.4f)
    { // Widen range slightly
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

float HardwareManager::getBatteryVoltage() const
{
    return currentSmoothedVoltage_;
}

uint8_t HardwareManager::getBatteryPercentage() const
{
    return calculatePercentage(currentSmoothedVoltage_);
}

bool HardwareManager::isCharging() const
{
    return isCharging_;
}

bool HardwareManager::isLaserOn() const
{
    return laserOn_;
}

bool HardwareManager::isVibrationOn() const
{
    return vibrationOn_;
}

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

InputEvent HardwareManager::getNextInputEvent()
{
    if (inputQueue_.empty())
    {
        return InputEvent::NONE;
    }
    InputEvent event = inputQueue_.front();
    inputQueue_.erase(inputQueue_.begin());

    // // LOG THE EVENT AS IT'S BEING PULLED FROM THE QUEUE
    // Serial.printf("[HW-LOG] Dequeuing event: %d\n", static_cast<int>(event));

    return event;
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

void HardwareManager::processEncoder()
{
    selectMux(Pins::MUX_CHANNEL_PCF0_ENCODER); // <-- MUX selection is INSIDE
    uint8_t pcf0State = readPCF(Pins::PCF0_ADDR);

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

    // Serial.printf("[ENC-LOG] State change: %d -> %d\n", lastEncState_, currentState);

    bool validCW = (currentState == cwTable[lastEncState_]);
    bool validCCW = (currentState == ccwTable[lastEncState_]);

    lastEncState_ = currentState;

    if (validCW)
    {
        if (encConsecutiveValid_ < 0)
            encConsecutiveValid_ = 0;
        encConsecutiveValid_++;
        // Serial.printf("[ENC-LOG] Valid CW step. Count: %d\n", encConsecutiveValid_);
    }
    else if (validCCW)
    {
        if (encConsecutiveValid_ > 0)
            encConsecutiveValid_ = 0;
        encConsecutiveValid_--;
        // Serial.printf("[ENC-LOG] Valid CCW step. Count: %d\n", encConsecutiveValid_);
    }
    else
    {
        encConsecutiveValid_ = 0;
        // Serial.println("[ENC-LOG] Invalid step. Counter reset.");
        return;
    }

    // *** THE FIX: Only queue events if the grace period is over ***
    if (millis() - setupTime_ < 300)
    {
        // We've updated the state, but we don't queue an event yet.
        return;
    }

    if (encConsecutiveValid_ >= requiredConsecutive)
    {
        inputQueue_.push_back(InputEvent::ENCODER_CW);
        // Serial.println("[ENC-LOG] ---> CW Event Queued! <---");
        encConsecutiveValid_ = 0;
    }
    else if (encConsecutiveValid_ <= -requiredConsecutive)
    {
        inputQueue_.push_back(InputEvent::ENCODER_CCW);
        // Serial.println("[ENC-LOG] ---> CCW Event Queued! <---");
        encConsecutiveValid_ = 0;
    }
}

void HardwareManager::processButton_PCF0()
{
    selectMux(Pins::MUX_CHANNEL_PCF0_ENCODER);
    uint8_t pcf0State = readPCF(Pins::PCF0_ADDR);

    // Only handles the Encoder Button on PCF0
    unsigned long currentTime = millis();
    static bool lastRawHState0[8] = {true, true, true, true, true, true, true, true};

    if (millis() - setupTime_ < 300)
        return;

    bool rawStateEncBtn = (pcf0State & (1 << Pins::ENC_BTN));
    if (rawStateEncBtn != lastRawHState0[Pins::ENC_BTN])
    {
        lastDbncT0_[Pins::ENC_BTN] = currentTime;
    }
    lastRawHState0[Pins::ENC_BTN] = rawStateEncBtn;

    if ((currentTime - lastDbncT0_[Pins::ENC_BTN]) > DEBOUNCE_DELAY_MS)
    {
        if (rawStateEncBtn != prevDbncHState0_[Pins::ENC_BTN])
        {
            prevDbncHState0_[Pins::ENC_BTN] = rawStateEncBtn;
            if (rawStateEncBtn == false)
            { // Pressed
                inputQueue_.push_back(InputEvent::BTN_ENCODER_PRESS);
            }
        }
    }
}

void HardwareManager::processButtons_PCF1()
{
    selectMux(Pins::MUX_CHANNEL_PCF1_NAV);
    uint8_t pcf1State = readPCF(Pins::PCF1_ADDR);

    // Handles all the Navigation Buttons on PCF1
    unsigned long currentTime = millis();
    static bool lastRawHState1[8] = {true, true, true, true, true, true, true, true};

    if (millis() - setupTime_ < 300)
        return;

    const int pcf1Pins[] = {Pins::NAV_OK, Pins::NAV_BACK, Pins::NAV_A, Pins::NAV_B, Pins::NAV_UP, Pins::NAV_DOWN, Pins::NAV_LEFT, Pins::NAV_RIGHT};

    for (size_t i = 0; i < sizeof(pcf1Pins) / sizeof(pcf1Pins[0]); ++i)
    {
        int pin = pcf1Pins[i];
        bool rawState = (pcf1State & (1 << pin));

        if (rawState != lastRawHState1[pin])
        {
            lastDbncT1_[pin] = currentTime;
        }
        lastRawHState1[pin] = rawState;

        if ((currentTime - lastDbncT1_[pin]) > DEBOUNCE_DELAY_MS)
        {
            if (rawState != prevDbncHState1_[pin])
            {
                prevDbncHState1_[pin] = rawState;
                if (rawState == false)
                { // Pressed
                    inputQueue_.push_back(mapPcf1PinToEvent(pin));
                    isBtnHeld1_[pin] = true;
                    btnHoldStartT1_[pin] = currentTime;
                    lastRepeatT1_[pin] = currentTime;
                }
                else
                { // Released
                    isBtnHeld1_[pin] = false;
                }
            }
        }
    }
}

// NEW function to handle auto-repeat
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

                inputQueue_.push_back(mapPcf1PinToEvent(pin));
                lastRepeatT1_[pin] = currentTime;
            }
        }
    }
}

// NEW helper to map a pin to an event
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