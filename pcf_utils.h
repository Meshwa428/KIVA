#ifndef PCF_UTILS_H
#define PCF_UTILS_H

#include <Arduino.h> // For uint8_t
#include "config.h"   // For MUX_ADDR, PCF0_ADDR, PCF1_ADDR

void selectMux(uint8_t channel);
void writePCF(uint8_t pcfAddress, uint8_t data);
uint8_t readPCF(uint8_t pcfAddress);
void setOutputOnPCF0(uint8_t pin, bool on); // Specific to PCF0 outputs

#endif // PCF_UTILS_H