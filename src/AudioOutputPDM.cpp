#include "AudioOutputPDM.h"
#include "Logger.h"

AudioOutputPDM::AudioOutputPDM(int pdm_pin, i2s_port_t i2s_port)
    : m_i2s_port(i2s_port),
      m_pdm_pin((gpio_num_t)pdm_pin),
      m_driver_installed(false),
      m_buffer(nullptr),
      m_buffer_ptr(0) {
}

AudioOutputPDM::~AudioOutputPDM() {
    stop();
    if (m_buffer) {
        free(m_buffer);
        m_buffer = nullptr;
    }
}

// begin() will now install the driver with a default rate to satisfy the MP3 decoder.
bool AudioOutputPDM::begin() {
    if (m_buffer == nullptr) {
        m_buffer = (int16_t*)malloc(BUFFER_FRAMES * sizeof(int16_t) * 2);
    }
    if (m_buffer == nullptr) {
        LOG(LogLevel::ERROR, "PDM", "Failed to allocate buffer");
        return false;
    }
    m_buffer_ptr = 0;

    // Install with a default rate. This will be immediately corrected by SetRate.
    this->hertz = 44100;
    install_i2s_driver();
    return true;
}

bool AudioOutputPDM::stop() {
    if (m_driver_installed) {
        // --- START OF FIX: More robust shutdown sequence ---
        // 1. Immediately stop the I2S hardware from processing its DMA buffer.
        i2s_stop(m_i2s_port);
        // 2. Clear any data that might be lingering in the DMA buffer.
        i2s_zero_dma_buffer(m_i2s_port);
        // 3. Now that the hardware is quiet, it's safe to uninstall the driver without blocking.
        i2s_driver_uninstall(m_i2s_port);
        // --- END OF FIX ---
        m_driver_installed = false;
        LOG(LogLevel::INFO, "PDM", "PDM Output driver stopped and uninstalled");
    }
    // Also clear our software buffer pointer
    m_buffer_ptr = 0;
    return true;
}

// SetRate() will now reinstall the driver with the correct sample rate.
bool AudioOutputPDM::SetRate(int hz) {
    if (this->hertz != hz) {
        LOG(LogLevel::INFO, "PDM", "Sample rate changed to %d Hz. Re-installing I2S driver.", hz);
        this->hertz = hz;
        // Uninstall the old driver
        if (m_driver_installed) {
            i2s_driver_uninstall(m_i2s_port);
        }
        // Install the new driver with the correct rate
        install_i2s_driver();
    }
    return true;
}

void AudioOutputPDM::install_i2s_driver() {
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_PDM),
        .sample_rate = (uint32_t)this->hertz,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 256,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };
    i2s_driver_install(m_i2s_port, &i2s_config, 0, NULL);

    i2s_pin_config_t i2s_pdm_pins = {
        .bck_io_num = I2S_PIN_NO_CHANGE,
        .ws_io_num = I2S_PIN_NO_CHANGE,
        .data_out_num = m_pdm_pin,
        .data_in_num = I2S_PIN_NO_CHANGE
    };
    i2s_set_pin(m_i2s_port, &i2s_pdm_pins);
    i2s_start(m_i2s_port);
    m_driver_installed = true;
}

bool AudioOutputPDM::ConsumeSample(int16_t sample[2]) {
    if (!m_buffer || !m_driver_installed) return false;

    int32_t mono_sample32 = (int32_t)sample[0] + (int32_t)sample[1];
    int16_t mono_sample = mono_sample32 / 2;

    // --- THIS IS THE FIX for Volume Control ---
    // Use the library's built-in gain mechanism.
    int16_t final_sample = Amplify(mono_sample);

    m_buffer[m_buffer_ptr * 2] = final_sample;
    m_buffer[m_buffer_ptr * 2 + 1] = final_sample;
    m_buffer_ptr++;

    if (m_buffer_ptr >= BUFFER_FRAMES) {
        flushBuffer();
    }
    return true;
}

void AudioOutputPDM::flushBuffer() {
    if (m_buffer_ptr > 0) {
        size_t bytes_written = 0;
        i2s_write(m_i2s_port, m_buffer, m_buffer_ptr * sizeof(int16_t) * 2, &bytes_written, portMAX_DELAY);
        m_buffer_ptr = 0;
    }
}