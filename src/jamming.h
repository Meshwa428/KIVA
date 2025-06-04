#ifndef JAMMING_H
#define JAMMING_H

#include "config.h" // For NRF_PINS, SPI_SPEED_NRF
#include <RF24.h>    // From the RF24 library

// Enum for different jamming modes
enum JammingType : uint8_t { // <--- ADDED : uint8_t HERE
    JAM_NONE,
    JAM_BLE,
    JAM_BT,
    JAM_NRF_CHANNELS,
    JAM_RF_NOISE_FLOOD
};

// RF24 Radio objects (will be extern as they are defined in jamming.cpp)
extern RF24 radio1;
extern RF24 radio2;

// Jamming control functions
void setupJamming();
bool startActiveJamming(JammingType type);
void stopActiveJamming();
void runJammerCycle(); // Called repeatedly by main loop when jamming

// To be defined in KivaMain.ino
extern JammingType activeJammingType;
extern bool isJammingOperationActive;

#endif // JAMMING_H