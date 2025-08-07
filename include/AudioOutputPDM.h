#ifndef AUDIO_OUTPUT_PDM_H
#define AUDIO_OUTPUT_PDM_H

#include "AudioOutput.h" // From ESP8266Audio library
#include <driver/i2s.h>

class AudioOutputPDM : public AudioOutput {
public:
    AudioOutputPDM(int pdm_pin, i2s_port_t i2s_port = I2S_NUM_0);
    virtual ~AudioOutputPDM();

    virtual bool SetRate(int hz) override;
    virtual bool ConsumeSample(int16_t sample[2]) override;
    virtual bool stop() override;
    virtual bool begin() override;

private:
    void flushBuffer();
    void install_i2s_driver(); // Helper function for installation

    i2s_port_t m_i2s_port;
    gpio_num_t m_pdm_pin;
    bool m_driver_installed; // Flag to track driver state

    int16_t* m_buffer;
    int m_buffer_ptr;
    static const int BUFFER_FRAMES = 512;
};

#endif // AUDIO_OUTPUT_PDM_H