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
        
        // Store previous smooth voltage for trend analysis
        float previousSmoothVoltage = currentBatteryVoltage;
        
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
        
        // 3) Enhanced charging detection
        const float CHARGING_THRESHOLD = 4.20f;
        const float VOLTAGE_RISE_THRESHOLD = 0.05f; // 50mV rise indicates charging
        const uint8_t PERCENTAGE_RISE_THRESHOLD = 2; // 2% charge increase indicates charging
        
        // Static variables for trend analysis (persist between calls)
        static float voltageChanges[5] = {0}; // Store last 5 voltage changes
        static uint8_t previousPercentages[5] = {0}; // Store last 5 percentage readings
        static int changeIndex = 0;
        static bool trendInitialized = false;
        
        // Calculate voltage change rate if we have previous data
        if (batInitialized && (currentTime - lastBatteryCheck > 0)) {
            float voltageChange = currentBatteryVoltage - previousSmoothVoltage;
            uint8_t currentPercentage = batPerc(currentBatteryVoltage);
            
            // Store voltage change and percentage for trend analysis
            voltageChanges[changeIndex] = voltageChange;
            previousPercentages[changeIndex] = currentPercentage;
            changeIndex = (changeIndex + 1) % 5;
            
            if (!trendInitialized && changeIndex == 0) {
                trendInitialized = true;
            }
            
            // Calculate trends
            float avgVoltageChange = 0;
            int8_t maxPercentageIncrease = 0;
            
            if (trendInitialized) {
                // Calculate average voltage change
                for (int i = 0; i < 5; i++) {
                    avgVoltageChange += voltageChanges[i];
                }
                avgVoltageChange /= 5.0f;
                
                // Find maximum percentage increase in recent samples
                uint8_t oldestPercentage = previousPercentages[changeIndex]; // Oldest reading
                maxPercentageIncrease = (int8_t)currentPercentage - (int8_t)oldestPercentage;
                
                // Detect charging based on multiple criteria:
                // 1. Voltage above threshold OR
                // 2. Sustained voltage rise OR  
                // 3. Percentage increase of 2% or more
                if (currentBatteryVoltage > CHARGING_THRESHOLD || 
                    avgVoltageChange > (VOLTAGE_RISE_THRESHOLD / 5.0f) ||
                    maxPercentageIncrease >= PERCENTAGE_RISE_THRESHOLD) {
                    isCharging = true;
                } else if (maxPercentageIncrease <= -PERCENTAGE_RISE_THRESHOLD || 
                          avgVoltageChange < -0.01f) {
                    // Clear charging if:
                    // 1. Percentage decreased by 2% or more OR
                    // 2. Clear negative voltage trend
                    isCharging = false;
                }
                // If trends are neutral, maintain previous state
            } else {
                // Fallback to simple threshold during initialization
                isCharging = (currentBatteryVoltage > CHARGING_THRESHOLD);
            }
        } else {
            // Initial state or no time passed - use simple threshold
            isCharging = (currentBatteryVoltage > CHARGING_THRESHOLD);
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