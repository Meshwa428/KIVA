#include "pcf_utils.h"
#include <Wire.h> // For I2C communication

// Definition of pcf0Output is in KivaMain.ino, accessed via extern in config.h
// If it were only used by this module, it could be defined here.

void selectMux(uint8_t channel) {
  if (channel > 7) return; // MUX TCA9548A has 8 channels (0-7)
  Wire.beginTransmission(MUX_ADDR);
  Wire.write(1 << channel);
  Wire.endTransmission();
  delayMicroseconds(350); // Allow MUX to settle, value might need tuning
}

void writePCF(uint8_t pcfAddress, uint8_t data) {
  Wire.beginTransmission(pcfAddress);
  Wire.write(data);
  Wire.endTransmission();
}

uint8_t readPCF(uint8_t pcfAddress) {
  uint8_t val = 0xFF; // Default to all high (inputs pulled up)
  if (Wire.requestFrom(pcfAddress, (uint8_t)1) == 1) {
    val = Wire.read();
  } else {
    // Error reading PCF - handle or log if necessary
    // Serial.print("Error reading PCF: 0x"); Serial.println(pcfAddress, HEX);
  }
  return val;
}

void setOutputOnPCF0(uint8_t pin, bool on) {
  // This function assumes PCF0 is for outputs like LEDs on specific pins
  // Pins 0-5 are assumed inputs for encoder/buttons based on defines
  if (pin < 6 || pin > 7) return; // Example: Only allow control of pins 6 and 7

  if (on) {
    pcf0Output |= (1 << pin);
  } else {
    pcf0Output &= ~(1 << pin);
  }
  selectMux(0); // MUX channel for PCF0
  writePCF(PCF0_ADDR, pcf0Output);
}