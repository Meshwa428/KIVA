#include "GameAudio.h"
#include "HardwareManager.h"
#include "Logger.h"
#include <Arduino.h>
#include <esp32-hal-ledc.h> // The ESP32's hardware timer/PWM library
#include <driver/gpio.h>     // Required for gpio_hold_dis/en to resolve hardware conflicts

// --- TUNING PARAMETERS ---
const int FADE_TIME_MS = 8;
const int LEDC_BASE_FREQ = 5000;
const int LEDC_RESOLUTION = 12;
const int DUTY_CYCLE_50_PERCENT = (1 << (LEDC_RESOLUTION - 1)); // 2048 for 12-bit

GameAudio::GameAudio() :
    hw_(nullptr),
    pin_(-1),
    isPlaying_(false),
    stopTime_(0)
{}

GameAudio::~GameAudio() {
    if (isPlaying_ && pin_ >= 0) {
        ledcDetach(pin_);
    }
}

void GameAudio::setup(HardwareManager* hw, int pin) {
    hw_ = hw;
    pin_ = pin;
    
    pinMode(pin_, OUTPUT);
    digitalWrite(pin_, LOW); // Ensure it starts off quiet

    LOG(LogLevel::INFO, "GAME_AUDIO", "LEDC GameAudio initialized for pin %d.", pin_);
}

void GameAudio::tone(uint32_t frequency, uint32_t duration) {
    if (!hw_ || pin_ < 0 || frequency == 0) return;

    // --- RAMPED SOFT-START SEQUENCE ---
    // 1. Release any 'hold' on the amplifier pin.
    gpio_hold_dis((gpio_num_t)pin_);
    
    // 2. Turn on the physical amplifier.
    hw_->setAmplifier(true);
    
    // 3. Attach the pin to an automatically assigned LEDC channel.
    ledcAttach(pin_, LEDC_BASE_FREQ, LEDC_RESOLUTION);
    
    // 4. Set ONLY the frequency. The duty cycle remains 0% (silent).
    ledcChangeFrequency(pin_, frequency, LEDC_RESOLUTION);
    
    // 5. Start a hardware-managed fade from 0% duty up to 50%.
    ledcFade(pin_, 0, DUTY_CYCLE_50_PERCENT, FADE_TIME_MS);

    isPlaying_ = true;
    stopTime_ = millis() + duration;
}

void GameAudio::noTone() {
    if (!isPlaying_ || !hw_ || pin_ < 0) return;

    // --- GRACEFUL FADE-OUT SHUTDOWN ---
    // 1. Start a hardware fade from the current duty down to 0% (silence).
    ledcFade(pin_, ledcRead(pin_), 0, FADE_TIME_MS);
    
    // 2. Wait for the short fade to complete. This is a small blocking delay,
    //    but it's acceptable for stopping a sound.
    delay(FADE_TIME_MS + 1);
    
    // 3. Detach the pin from the LEDC peripheral.
    ledcDetach(pin_);

    // 4. Drive the pin LOW manually.
    digitalWrite(pin_, LOW);
    
    // 5. Turn off the physical amplifier now that the signal is gone.
    hw_->setAmplifier(false);
    
    // 6. Latch the pin in the LOW state to prevent noise.
    gpio_hold_en((gpio_num_t)pin_);
    
    isPlaying_ = false;
}

void GameAudio::update() {
    // This is called from the main App::loop() to handle tone durations
    // in a non-blocking way.
    if (isPlaying_ && millis() >= stopTime_) {
        noTone();
    }
}