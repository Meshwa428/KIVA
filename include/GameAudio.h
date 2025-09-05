#ifndef GAME_AUDIO_H
#define GAME_AUDIO_H

#include <cstdint>

// Forward declaration
class HardwareManager;

class GameAudio {
public:
    GameAudio();

    /**
     * @brief Sets up the LEDC peripheral for generating tones. Must be called once.
     * @param hw A pointer to the main HardwareManager to control the amplifier.
     * @param pin The GPIO pin connected to the speaker/amplifier.
     * @param channel The LEDC channel to use (0-15).
     */
    void setup(HardwareManager* hw, int pin, int channel = 0);

    /**
     * @brief Plays a tone of a given frequency for a specific duration. This is non-blocking.
     * @param frequency The frequency of the tone in Hz.
     * @param duration The duration of the tone in milliseconds.
     */
    void tone(uint32_t frequency, uint32_t duration);

    /**
     * @brief Immediately stops any playing tone.
     */
    void noTone();

    /**
     * @brief Must be called in the main loop to handle tone durations.
     */
    void update();

private:
    HardwareManager* hw_;
    int pin_;
    int channel_;
    bool isPlaying_;
    unsigned long stopTime_;
};

#endif // GAME_AUDIO_H