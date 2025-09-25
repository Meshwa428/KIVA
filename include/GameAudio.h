#ifndef GAME_AUDIO_H
#define GAME_AUDIO_H

#include <cstdint>

// Forward declarations
class App; // <-- ADD this
class HardwareManager;

#include "Service.h"

class GameAudio : public Service {
public:
    GameAudio();
    ~GameAudio();

    /**
     * @brief Prepares the GameAudio manager.
     * @param app A pointer to the main App object to access settings.
     * @param pin The GPIO pin connected to the speaker/amplifier's audio input.
     */
    void setup(App* app) override;

    /**
     * @brief Plays a tone of a given frequency for a specific duration with a pop-free fade-in.
     * @param frequency The frequency of the tone in Hz.
     * @param duration The duration of the tone in milliseconds.
     */
    void tone(uint32_t frequency, uint32_t duration);

    /**
     * @brief Immediately stops any playing tone with a pop-free fade-out.
     */
    void noTone();

    /**
     * @brief Must be called in the main application loop to handle tone durations.
     */
    void update();

private:
    App* app_; // <-- MODIFIED from HardwareManager* to App*
    int pin_;
    bool isPlaying_;
    unsigned long stopTime_;
};

#endif // GAME_AUDIO_H