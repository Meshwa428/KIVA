#ifndef GAME_AUDIO_H
#define GAME_AUDIO_H

#include <cstdint>

// Forward declaration
class HardwareManager;

class GameAudio {
public:
    GameAudio();
    ~GameAudio();

    /**
     * @brief Prepares the GameAudio manager.
     * @param hw A pointer to the HardwareManager to control the amplifier.
     * @param pin The GPIO pin connected to the speaker/amplifier's audio input.
     */
    void setup(HardwareManager* hw, int pin);

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
    HardwareManager* hw_;
    int pin_;
    bool isPlaying_;
    unsigned long stopTime_;
};

#endif // GAME_AUDIO_H