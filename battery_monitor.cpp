#include "battery_monitor.h"
#include <Arduino.h> // For analogRead

void setupBatteryMonitor() {
    // If ADC_PIN is valid, set attenuation (ESP32 specific)
    // This is usually done in main setup(), but can be here if isolated.
    // For now, assume main setup handles analogReadResolution and attenuation.
    batteryNeedsUpdate = true; // Force initial read
}

float readBatV() {
  // Ensure analogReadResolution(ADC_RES_BITS); and analogSetPinAttenuation()
  // have been called in the main setup() if required for your board.
  // ESP32: analogReadResolution(12) for 0-4095;
  //        analogSetPinAttenuation(ADC_PIN, ADC_11db) for up to ~3.3V
  int rawValue = analogRead(ADC_PIN);
  float voltageAtADC = (rawValue / (float)ADC_RES) * ADC_REF;
  float batteryVoltage = voltageAtADC * VOLTAGE_RATIO;

  // Basic sanity check for battery voltage
  if (batteryVoltage < 2.5f || batteryVoltage > 5.0f) {
    //Serial.print("Invalid battery voltage: "); Serial.println(batteryVoltage);
    return lastValidBattery; // Return the last known good reading
  }
  lastValidBattery = batteryVoltage;
  return batteryVoltage;
}

float getSmoothV() {
  unsigned long currentTime = millis();
  if (batteryNeedsUpdate || (currentTime - lastBatteryCheck >= BATTERY_CHECK_INTERVAL)) {
    float currentVoltageReading = readBatV();

    if (!batInitialized) {
      for (int i = 0; i < BAT_SAMPLES; i++) {
        batReadings[i] = currentVoltageReading; // Prime with current reading
      }
      batInitialized = true;
    }

    batReadings[batIndex] = currentVoltageReading;
    batIndex = (batIndex + 1) % BAT_SAMPLES;

    float weightedSum = 0;
    float totalWeight = 0;
    // Weighted moving average (more recent readings have higher weight)
    for (int i = 0; i < BAT_SAMPLES; i++) {
      float weight = 1.0f + (float)i / BAT_SAMPLES; // Simple linear weight
      int historicIndex = (batIndex - 1 - i + BAT_SAMPLES) % BAT_SAMPLES;
      weightedSum += batReadings[historicIndex] * weight;
      totalWeight += weight;
    }

    if (totalWeight > 0) {
      currentBatteryVoltage = weightedSum / totalWeight;
    } else {
      currentBatteryVoltage = currentVoltageReading; // Fallback if weights are zero
    }
    
    lastBatteryCheck = currentTime;
    batteryNeedsUpdate = false; // Reset flag
    //Serial.print("Smoothed Battery Voltage: "); Serial.println(currentBatteryVoltage);
  }
  return currentBatteryVoltage;
}

uint8_t batPerc(float voltage) {
  // Voltage range for LiPo/Li-ion: 3.0V (empty) to 4.2V (full)
  // Using 3.7V as a nominal midpoint, but percentage is often non-linear.
  // This is a simplified linear mapping.
  const float minVoltage = 3.2f; // Adjust as per your battery's "empty" point
  const float maxVoltage = 4.2f; // Full charge
  
  float percentage = (voltage - minVoltage) / (maxVoltage - minVoltage) * 100.0f;

  if (percentage < 0) percentage = 0;
  if (percentage > 100) percentage = 100;

  return (uint8_t)percentage;
}