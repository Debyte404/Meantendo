#include "Buzzer.hpp"

// ===== CONFIG =====
static constexpr uint8_t BUZZER_PIN = 15;
static constexpr uint8_t BUZZER_CH  = 4;    // Channel 4 — avoids SPI timer conflicts
// ==================

namespace Buzzer {

/**
 * Attach → tone → detach cycle.
 * GPIO 15 is a strapping pin; leaving LEDC permanently attached
 * interferes with SPI after the first beep, killing the display.
 * By attaching only for the duration of the beep and immediately
 * detaching, we release the pin back to normal GPIO state.
 */
static void beep(uint16_t freq, uint16_t duration) {
    ledcSetup(BUZZER_CH, freq, 8);
    ledcAttachPin(BUZZER_PIN, BUZZER_CH);
    ledcWrite(BUZZER_CH, 128);          // 50% duty
    delay(duration);
    ledcWrite(BUZZER_CH, 0);            // silence
    ledcDetachPin(BUZZER_PIN);          // KEY: release GPIO back
    pinMode(BUZZER_PIN, OUTPUT);        // reset to safe output
    digitalWrite(BUZZER_PIN, LOW);      // pull low — no floating
    delayMicroseconds(200);             // let bus settle
}

void init() {
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    // No permanent ledcAttachPin here — that's what caused the SPI conflict
}

void play(Sound s) {
    switch (s) {
        case Sound::Click:
            beep(2000, 25);
            break;

        case Sound::Confirm:
            beep(1500, 60);
            break;

        case Sound::Error:
            beep(700, 180);
            break;
    }
}

}
