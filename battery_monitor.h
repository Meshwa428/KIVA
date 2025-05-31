#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include "config.h" // For ADC defines, BAT_SAMPLES etc.
#include <Arduino.h> // For uint8_t

// Extern global variables related to battery (defined in KivaMain.ino)
extern float batReadings[BAT_SAMPLES];
extern int batIndex;
extern bool batInitialized;
extern float lastValidBattery;
extern unsigned long lastBatteryCheck;
extern float currentBatteryVoltage;
extern bool batteryNeedsUpdate;

// Function declarations
void setupBatteryMonitor(); // Optional, if specific init needed beyond ADC setup
float readBatV();
float getSmoothV();
uint8_t batPerc(float voltage);

#endif // BATTERY_MONITOR_H