#include "GameAudio.h"
#include "App.h"
#include "ConfigManager.h"
#include "HardwareManager.h"
#include "Logger.h"
#include <Arduino.h>
#include <algorithm>
#include <esp32-hal-ledc.h>
#include <driver/gpio.h>

// --- TUNING PARAMETERS ---
const int FADE_TIME_MS = 50;
const int LEDC_BASE_FREQ = 5000;
const int LEDC_RESOLUTION = 12;
const int DUTY_CYCLE_MAX = (1 << (LEDC_RESOLUTION - 1)); // 2048 for 12-bit (50% duty is max for a tone)

GameAudio::GameAudio() :
    app_(nullptr),
    pin_(-1),
    isPlaying_(false),
    stopTime_(0)
{}

GameAudio::~GameAudio() {
    if (isPlaying_ && pin_ >= 0) {
        ledcDetach(pin_);
    }
}

#include "Config.h"

void GameAudio::setup(App* app) {
    app_ = app;
    pin_ = Pins::AMPLIFIER_PIN;

    pinMode(pin_, OUTPUT);
    digitalWrite(pin_, LOW); // Ensure it starts off quiet

    LOG(LogLevel::INFO, "GAME_AUDIO", "LEDC GameAudio initialized for pin %d on channel 0.", pin_);
}

void GameAudio::tone(uint32_t frequency, uint32_t duration) {
    if (!app_ || pin_ < 0 || frequency == 0) return;

    uint8_t volume_percent = app_->getConfigManager().getSettings().volume;
    uint8_t effective_volume = std::min(volume_percent, (uint8_t)100);
    uint32_t target_duty = (DUTY_CYCLE_MAX * effective_volume) / 100;
    
    if (target_duty == 0) return;

    gpio_hold_dis((gpio_num_t)pin_);
    app_->getHardwareManager().setAmplifier(true);

    ledcAttach(pin_, LEDC_BASE_FREQ, LEDC_RESOLUTION);
    ledcChangeFrequency(pin_, frequency, LEDC_RESOLUTION);
    
    // Fade in to the calculated target duty cycle.
    ledcFade(pin_, 0, target_duty, FADE_TIME_MS);

    isPlaying_ = true;
    stopTime_ = millis() + duration;
}

void GameAudio::noTone() {
    if (!isPlaying_ || !app_ || pin_ < 0) return;

    // --- GRACEFUL FADE-OUT SHUTDOWN ---
    ledcFade(pin_, ledcRead(pin_), 0, FADE_TIME_MS);
    delay(FADE_TIME_MS + 1);
    
    ledcDetach(pin_);
    digitalWrite(pin_, LOW);
    app_->getHardwareManager().setAmplifier(false);
    gpio_hold_en((gpio_num_t)pin_);
    
    isPlaying_ = false;
}

void GameAudio::loop() {
    if (isPlaying_ && millis() >= stopTime_) {
        noTone();
    }
}