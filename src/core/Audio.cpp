#include "Audio.hpp"
#include <driver/i2s.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

static const char* TAG = "Audio";

#define CHECK_I2S(x) do { \
    esp_err_t __err = (x); \
    if (__err != ESP_OK) { \
        ESP_LOGE(TAG, "I2S Error %s: %s", #x, esp_err_to_name(__err)); \
    } \
} while (0)

void MeantendoAudio::init() {
    ESP_LOGI(TAG, "Initializing Audio...");

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, // Interleaved stereo
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 512, // Matches DOOM mix buffer size somewhat
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCK_IO,
        .ws_io_num = I2S_WS_IO,
        .data_out_num = I2S_DO_IO,
        .data_in_num = I2S_DI_IO
    };

    CHECK_I2S(i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL));
    CHECK_I2S(i2s_set_pin(I2S_NUM_0, &pin_config));
    CHECK_I2S(i2s_zero_dma_buffer(I2S_NUM_0));

    ESP_LOGI(TAG, "Audio initialized successfully");
}

void MeantendoAudio::submitBuffer(const int16_t* buffer, size_t sampleCount) {
    size_t bytes_written = 0;
    // sampleCount is number of int16 samples (L+R interleaved)
    // Total bytes = sampleCount * sizeof(int16_t)
    size_t bytes_to_write = sampleCount * sizeof(int16_t);
    
    // Write to I2S DMA buffer
    // This is blocking if the buffer is full, which helps throttle DOOM's loop
    ESP_ERROR_CHECK_WITHOUT_ABORT(i2s_write(I2S_NUM_0, buffer, bytes_to_write, &bytes_written, portMAX_DELAY));
}

void MeantendoAudio::stop() {
    i2s_driver_uninstall(I2S_NUM_0);
}

extern "C" {
    void meantendo_audio_init(void) {
        MeantendoAudio::init();
    }

    void meantendo_audio_submit_buffer(const int16_t* buffer, int sample_count) {
        MeantendoAudio::submitBuffer(buffer, (size_t)sample_count);
    }

    void meantendo_audio_stop(void) {
        MeantendoAudio::stop();
    }
}
