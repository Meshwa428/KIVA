#include <Arduino.h>
#include "App.h"

// Get the singleton instance of the App
App& kivaApp = App::getInstance();

void setup() {
  kivaApp.setup();
}

void loop() {
  kivaApp.loop();
}