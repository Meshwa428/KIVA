// In KivaMain/battery_monitor.cpp
#include "battery_monitor.h"
#include <Arduino.h>

// setupBatteryMonitor() and readBatV() remain the same.
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

  // --- Constants for Charging Detection Logic ---
  const float CHARGING_FLOAT_VOLTAGE_THRESHOLD = 4.16f; // Voltage considered "high enough" for float/full
  const float CHARGING_ACTIVE_MIN_VOLTAGE = 3.5f;      // Min voltage for active charging detection if rising
  const float MIN_SUSTAINED_VOLTAGE_RISE_TOTAL = 0.015f; // Total rise over TREND_SAMPLES_WINDOW to initiate charging
  const int TREND_SAMPLES_WINDOW = 3;                 // Samples for rise detection
  const int STABLE_HIGH_CONFIRM_COUNT = 3;            // How many samples at high voltage to confirm "charging/full"
  // How much voltage can drop from the last known "charging peak" or "stable high" before we decide it's no longer charging.
  const float MAX_VOLTAGE_DROP_FROM_STABLE_HIGH_OR_PEAK = 0.020f; // 20mV. Increased slightly for more stability against noise when full.

  // Static variables
  static float trend_voltages[TREND_SAMPLES_WINDOW] = {0};
  static int trend_idx = 0;
  static bool trend_buffer_filled = false;
  
  static float reference_voltage_for_charging_check = 0.0f; // Stores voltage when isCharging became true, or highest V during current charge, or stable high V.
  static int stable_high_samples_count = 0; 

  if (batteryNeedsUpdate || (currentTime - lastBatteryCheck >= BATTERY_CHECK_INTERVAL)) {
    float currentRawVoltageReading = readBatV();
    float newCalculatedSmoothVoltage; 

    if (!batInitialized) {
      for (int i = 0; i < BAT_SAMPLES; i++) batReadings[i] = currentRawVoltageReading; 
      for (int i = 0; i < TREND_SAMPLES_WINDOW; i++) trend_voltages[i] = currentRawVoltageReading; 
      batInitialized = true;
      newCalculatedSmoothVoltage = currentRawVoltageReading; 
      reference_voltage_for_charging_check = newCalculatedSmoothVoltage; 
    } else {
      batReadings[batIndex] = currentRawVoltageReading;
      batIndex = (batIndex + 1) % BAT_SAMPLES;

      float weightedSum = 0;
      float totalWeight = 0;
      for (int i = 0; i < BAT_SAMPLES; i++) {
        float weight = 1.0f + (float)i / BAT_SAMPLES; 
        int historicIndex = (batIndex - 1 - i + BAT_SAMPLES) % BAT_SAMPLES;
        weightedSum += batReadings[historicIndex] * weight;
        totalWeight += weight;
      }
      newCalculatedSmoothVoltage = (totalWeight > 0) ? (weightedSum / totalWeight) : currentRawVoltageReading;
    }
    
    currentBatteryVoltage = newCalculatedSmoothVoltage;

    trend_voltages[trend_idx] = currentBatteryVoltage;
    trend_idx = (trend_idx + 1) % TREND_SAMPLES_WINDOW;
    if (!trend_buffer_filled && trend_idx == 0) trend_buffer_filled = true; 

    // --- Charging State Inference ---
    bool prev_isCharging_state = isCharging;

    if (isCharging) {
        // If we previously thought it was charging:
        // Continuously update the reference voltage to the current (or highest seen) voltage if it's not dropping.
        // This means reference_voltage_for_charging_check tracks the "stable" or "climbing" point.
        if (currentBatteryVoltage >= reference_voltage_for_charging_check - (MAX_VOLTAGE_DROP_FROM_STABLE_HIGH_OR_PEAK / 2.0f) ) { // allow tiny dip before updating ref down
             reference_voltage_for_charging_check = max(reference_voltage_for_charging_check, currentBatteryVoltage);
        }
       
        // Check if voltage has dropped significantly from this reference.
        if (currentBatteryVoltage < (reference_voltage_for_charging_check - MAX_VOLTAGE_DROP_FROM_STABLE_HIGH_OR_PEAK)) {
            isCharging = false;
            // When charging stops due to a drop, reset related state
            stable_high_samples_count = 0; 
            // reference_voltage_for_charging_check will be re-evaluated when trying to enter charging again
        }
    } else { // Was NOT charging, check if it should START charging
        if (trend_buffer_filled) {
            float oldest_v_in_trend = trend_voltages[trend_idx]; 
            float voltage_trend_over_window = currentBatteryVoltage - oldest_v_in_trend;

            // Condition 1: Actively rising voltage
            if (voltage_trend_over_window > MIN_SUSTAINED_VOLTAGE_RISE_TOTAL && currentBatteryVoltage > CHARGING_ACTIVE_MIN_VOLTAGE) {
                isCharging = true;
            } 
            // Condition 2: Stable high voltage
            else if (currentBatteryVoltage >= CHARGING_FLOAT_VOLTAGE_THRESHOLD) {
                stable_high_samples_count++;
                if (stable_high_samples_count >= STABLE_HIGH_CONFIRM_COUNT) {
                    isCharging = true; 
                }
            } else {
                // Voltage is not high and not actively rising significantly
                stable_high_samples_count = 0; 
            }
        } else { // Not enough trend data yet
            if (currentBatteryVoltage >= CHARGING_FLOAT_VOLTAGE_THRESHOLD) {
                 isCharging = true; 
                 stable_high_samples_count = STABLE_HIGH_CONFIRM_COUNT; 
            } else {
                 isCharging = false;
                 stable_high_samples_count = 0;
            }
        }
        
        // If isCharging just became true, set/update the reference point.
        if (isCharging && !prev_isCharging_state) {
            reference_voltage_for_charging_check = currentBatteryVoltage;
            // If it became true due to stable_high, stable_high_samples_count is already >= threshold.
            // If due to rise, stable_high_samples_count was reset/is 0.
        }
    }

    // Final override: If battery is critically low, it cannot be considered charging.
    if (currentBatteryVoltage < 3.1f) {
        if (isCharging) { // If it thought it was charging but voltage is now critical
            isCharging = false;
        }
        stable_high_samples_count = 0;
        // When critically low, the reference voltage should reflect this low state
        // to allow detection of a rise if charger is connected to a very dead battery.
        reference_voltage_for_charging_check = currentBatteryVoltage; 
    }
    
    lastBatteryCheck = currentTime;
    batteryNeedsUpdate = false; 

    // Serial.print("V: "); Serial.print(currentBatteryVoltage, 3);
    // Serial.print(" | isChg: "); Serial.print(isCharging ? "Y" : "N");
    // Serial.print(" | RefV: "); Serial.print(reference_voltage_for_charging_check, 3);
    // Serial.print(" | stableCnt: "); Serial.println(stable_high_samples_count);
  }
  return currentBatteryVoltage;
}

// batPerc() function remains the same.
uint8_t batPerc(float voltage) {
  const float minVoltage = 3.2f; 
  const float maxVoltage = 4.2f; 
  float percentage;
  if (isCharging && voltage > maxVoltage) { 
      percentage = 100.0f;
  } else {
     percentage = (voltage - minVoltage) / (maxVoltage - minVoltage) * 100.0f;
  }
  if (percentage < 0) percentage = 0;
  if (percentage > 100) percentage = 100;
  return (uint8_t)percentage;
}