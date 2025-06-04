// In KivaMain/battery_monitor.cpp
#include "battery_monitor.h"
#include <Arduino.h>

void setupBatteryMonitor() {
    batteryNeedsUpdate = true;
}

float readBatV() {
    int rawValue = analogRead(ADC_PIN);
    float voltageAtADC = (rawValue / (float)ADC_RES) * ADC_REF;
    float batteryVoltage = voltageAtADC * VOLTAGE_RATIO;

    if (batteryVoltage < 2.5f || batteryVoltage > 4.35f) {
        return lastValidBattery;
    }
    lastValidBattery = batteryVoltage;
    return batteryVoltage;
}

float getSmoothV() {
    unsigned long currentTime = millis();

    if (batteryNeedsUpdate || (currentTime - lastBatteryCheck >= BATTERY_CHECK_INTERVAL)) {
        // 1) Read raw calibrated battery voltage
        float currentRawVoltageReading = readBatV();

        // 2) Smoothing (weighted moving average)
        float newCalculatedSmoothVoltage;
        if (!batInitialized) {
            for (int i = 0; i < BAT_SAMPLES; i++) {
                batReadings[i] = currentRawVoltageReading;
            }
            batInitialized = true;
            newCalculatedSmoothVoltage = currentRawVoltageReading;
        } else {
            batReadings[batIndex] = currentRawVoltageReading;
            batIndex = (batIndex + 1) % BAT_SAMPLES;

            float weightedSum = 0;
            float totalWeight = 0;
            for (int i = 0; i < BAT_SAMPLES; i++) {
                float weight = 1.0f + (float)i / BAT_SAMPLES;
                int idx = (batIndex - 1 - i + BAT_SAMPLES) % BAT_SAMPLES;
                weightedSum += batReadings[idx] * weight;
                totalWeight += weight;
            }
            newCalculatedSmoothVoltage = (totalWeight > 0)
                                         ? (weightedSum / totalWeight)
                                         : currentRawVoltageReading;
        }

        currentBatteryVoltage = newCalculatedSmoothVoltage;

        // 3) Charging detection: true whenever voltage > 4.20 V
        const float CHARGING_THRESHOLD = 4.20f;
        if (currentBatteryVoltage > CHARGING_THRESHOLD) {
            isCharging = true;
        } else {
            isCharging = false;
        }

        lastBatteryCheck = currentTime;
        batteryNeedsUpdate = false;
    }

    return currentBatteryVoltage;
}

uint8_t batPerc(float voltage) {
    // If battery is between 4.15 V and 4.20 V, show 100%
    if (voltage >= 4.15f) {
        return 100;
    }

    // Otherwise, map 3.2 .. 4.15 V → 0 .. 100%
    const float minVoltage = 3.2f;
    const float maxVoltage = 4.15f;
    float percentage = (voltage - minVoltage) / (maxVoltage - minVoltage) * 100.0f;

    if (percentage < 0) percentage = 0;
    if (percentage > 100) percentage = 100;
    return (uint8_t)percentage;
}