#ifndef INPUT_HANDLING_H
#define INPUT_HANDLING_H

#include "config.h" // For QuadratureEncoder struct, button defines, MenuState

// No extern global variables needed here specifically, they are in config.h
// Debouncing arrays will be static within input_handling.cpp

void setupInputs(); // Call this from main setup()
void updateInputs();
void scrollAct(int direction, bool isGridContext, bool isCarouselContext);
const char* getPCF1BtnName(int pin); // Helper

// QuadratureEncoder object will be defined in KivaMain.ino and externed
extern QuadratureEncoder encoder;

// Button press flags (defined in KivaMain.ino)
extern bool btnPress0[8];
extern bool btnPress1[8];

#endif // INPUT_HANDLING_H