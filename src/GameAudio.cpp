#include "GameAudio.h"
#include "HardwareManager.h"
#include <Arduino.h> // For millis()
#include <esp32-hal-ledc.h> // For ledc functions

GameAudio::GameAudio() :
    hw_(nullptr),
    pin_(-1),
    channel_(0),
    isPlaying_(false),
    stopTime_(0)
{}

void GameAudio::setup(HardwareManager* hw, int pin, int channel) {
    hw_ = hw;
    pin_ = pin;
    channel_ = channel;

    ledcAttach(pin_, 5000, 12);
    ledcWrite(channel_, 0); // Start with duty 0 (off)
}

void GameAudio::tone(uint32_t frequency, uint32_t duration) {
    if (!hw_) return;

    hw_->setAmplifier(true);

    if (frequency == 0) { // A frequency of 0 is a rest
        ledcWrite(channel_, 0);
    } else {
        ledcWriteTone(channel_, frequency);
        // Set duty to 50% for a standard square wave. 12-bit res = 4096. 50% = 2048
        ledcWrite(channel_, 2048); 
    }
    
    isPlaying_ = true;
    stopTime_ = millis() + duration;
}

void GameAudio::noTone() {
    if (!hw_) return;
    
    ledcWrite(channel_, 0); // Set duty cycle to 0 to stop sound
    hw_->setAmplifier(false);
    isPlaying_ = false;
}

void GameAudio::update() {
    if (isPlaying_ && millis() >= stopTime_) {
        noTone();
    }
}