#ifndef MEANTENDO_AUDIO_HPP
#define MEANTENDO_AUDIO_HPP

#include <stdint.h>
#include <stddef.h>

// I2S Configuration
#define I2S_BCK_IO      26
#define I2S_WS_IO       25
#define I2S_DO_IO       22
#define I2S_DI_IO       -1 // Not used

// Audio format
#define SAMPLE_RATE     11025
#define SAMPLE_BITS     16

#ifdef __cplusplus
// We are calling this MeantendoAudio to avoid conflicts with other libraries
class MeantendoAudio {
public:
    static void init();
    static void submitBuffer(const int16_t* buffer, size_t sampleCount);
    static void stop();
};

extern "C" {
#endif

void meantendo_audio_init(void);
void meantendo_audio_submit_buffer(const int16_t* buffer, int sample_count);
void meantendo_audio_stop(void);

#ifdef __cplusplus
}
#endif

#endif // MEANTENDO_AUDIO_HPP
