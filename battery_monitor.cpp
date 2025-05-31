// In KivaMain/battery_monitor.cpp
#include "battery_monitor.h"
#include <Arduino.h>

// setupBatteryMonitor() and readBatV() remain the same as your current working versions.
// Ensure readBatV() provides accurate raw voltage readings.
void setupBatteryMonitor() {
    batteryNeedsUpdate = true; 
}

float readBatV() {
  int rawValue = analogRead(ADC_PIN);
  float voltageAtADC = (rawValue / (float)ADC_RES) * ADC_REF;
  float batteryVoltage = voltageAtADC * VOLTAGE_RATIO;

  // Basic sanity check for battery voltage
  // Adjusted max for potential charging voltage slightly above 4.2V
  if (batteryVoltage < 2.5f || batteryVoltage > 4.35f) { 
    //Serial.print("Invalid battery voltage: "); Serial.println(batteryVoltage);
    return lastValidBattery; 
  }
  lastValidBattery = batteryVoltage;
  return batteryVoltage;
}


float getSmoothV() {
  unsigned long currentTime = millis();

  // Thresholds for charging detection (can be tuned)
  const float CHARGING_VOLTAGE_THRESHOLD = 4.18f; // Voltage above which we consider it potentially charging if stable/rising
  const float FULLY_CHARGED_VOLTAGE = 4.15f;    // Voltage around which it's considered full
  const float MIN_VOLTAGE_RISE_DETECT = 0.005f; // Minimum rise in voltage over samples to suspect charging
  const int CHARGING_DETECT_SAMPLES = 5;       // Check trend over last 5 smoothed readings (e.g., 5 seconds)
  static float prev_smoothed_voltages[CHARGING_DETECT_SAMPLES] = {0};
  static int charge_detect_idx = 0;
  static bool charge_detect_buffer_filled = false;


  if (batteryNeedsUpdate || (currentTime - lastBatteryCheck >= BATTERY_CHECK_INTERVAL)) {
    float currentRawVoltageReading = readBatV();

    if (!batInitialized) {
      for (int i = 0; i < BAT_SAMPLES; i++) {
        batReadings[i] = currentRawVoltageReading; 
      }
      for (int i = 0; i < CHARGING_DETECT_SAMPLES; i++) {
        prev_smoothed_voltages[i] = currentRawVoltageReading; // Prime with initial reading
      }
      batInitialized = true;
    }

    // Update smoothing buffer for currentBatteryVoltage
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

    if (totalWeight > 0) {
      currentBatteryVoltage = weightedSum / totalWeight;
    } else {
      currentBatteryVoltage = currentRawVoltageReading; 
    }
    
    // --- Charging State Inference ---
    prev_smoothed_voltages[charge_detect_idx] = currentBatteryVoltage;
    charge_detect_idx = (charge_detect_idx + 1) % CHARGING_DETECT_SAMPLES;
    if (!charge_detect_buffer_filled && charge_detect_idx == 0) {
        charge_detect_buffer_filled = true; // Buffer is now full for the first time
    }

    if (charge_detect_buffer_filled) {
        float oldest_v = prev_smoothed_voltages[charge_detect_idx]; // Oldest is next to be overwritten
        float newest_v = currentBatteryVoltage; // Which is prev_smoothed_voltages[(charge_detect_idx - 1 + CHARGING_DETECT_SAMPLES) % CHARGING_DETECT_SAMPLES]
        
        float voltage_trend = newest_v - oldest_v;

        // Condition 1: Voltage is very high (typical of end-of-charge or float charge)
        if (newest_v >= CHARGING_VOLTAGE_THRESHOLD) {
            isCharging = true;
        }
        // Condition 2: Sustained voltage rise over the detection window
        else if (voltage_trend > MIN_VOLTAGE_RISE_DETECT * CHARGING_DETECT_SAMPLES && newest_v > 3.5f) { // Ensure not just noise at low V
            // If voltage has risen significantly, likely charging
            isCharging = true;
        }
        // Condition 3: Voltage is high and stable (fully charged and plugged in)
        else if (isCharging && newest_v >= FULLY_CHARGED_VOLTAGE && abs(voltage_trend) < MIN_VOLTAGE_RISE_DETECT) {
            // Was charging, voltage is high and stable, assume still plugged in & charged
            isCharging = true;
        }
        // Condition 4: If voltage drops significantly, it's likely discharging
        else if (voltage_trend < -(MIN_VOLTAGE_RISE_DETECT * 1.5f)) { // More sensitive to drops
            isCharging = false;
        }
        // else: maintain current charging state if uncertain (e.g. stable voltage not near full)
        
    } else {
        // Not enough data yet to determine charging state reliably, assume not charging or use initial guess
        // A simple initial guess if voltage is already high:
        if (currentBatteryVoltage > CHARGING_VOLTAGE_THRESHOLD) {
            isCharging = true;
        } else {
            isCharging = false;
        }
    }
    // If battery voltage is very low, it cannot be charging (unless it's a dead battery trying to recover)
    if (currentBatteryVoltage < 3.0f) {
        isCharging = false;
    }


    lastBatteryCheck = currentTime;
    batteryNeedsUpdate = false; 
    // Serial.print("Smoothed V: "); Serial.print(currentBatteryVoltage);
    // Serial.print(" | Charging: "); Serial.println(isCharging ? "YES" : "NO");
  }
  return currentBatteryVoltage;
}

// batPerc() function remains the same
uint8_t batPerc(float voltage) {
  const float minVoltage = 3.2f; 
  const float maxVoltage = 4.2f; 
  
  float percentage;
  if (isCharging && voltage > maxVoltage) { // If charging and voltage goes slightly above "full"
      percentage = 100.0f;
  } else if (!isCharging && voltage > maxVoltage) { // If not charging but voltage is somehow above max (e.g. fresh off charger)
      percentage = 100.0f;
  }
  else {
     percentage = (voltage - minVoltage) / (maxVoltage - minVoltage) * 100.0f;
  }

  if (percentage < 0) percentage = 0;
  if (percentage > 100) percentage = 100;

  return (uint8_t)percentage;
}