#pragma once
#include <Arduino.h>
#include <esp_sleep.h>

/**
 * ═════════════════════════════════════════════════════════════════════════
 *  Meantendo Power & Transient State Persistence Engine
 *  - Uses 8KB RTC Fast/Slow SRAM (preserved across deep sleep & brownouts)
 *  - Instant pitch-black screen sleep (<1mA) with instant wake on Back / A
 *  - Zero white-screen flash on sleep
 * ═════════════════════════════════════════════════════════════════════════
 */

struct __attribute__((packed)) MeantendoSavedState {
    uint32_t magic;           // 0x4D45414E ("MEAN")
    uint8_t  activeGameId;    // 0-255
    uint32_t score;           // saved score
    uint32_t highScore;       // persistent high score
    uint16_t gameData[32];    // arbitrary game registers (positions, snake body, etc.)
    uint32_t saveTimestamp;   // millis()
    uint16_t checksum;
};

class PowerStateEngine {
public:
    static constexpr uint32_t STATE_MAGIC = 0x4D45414E; // "MEAN"

    void init();
    bool hasSavedState();
    bool save(uint8_t gameId, uint32_t score, const uint16_t* extraData = nullptr, size_t extraLen = 0);
    bool restore(uint8_t &outGameId, uint32_t &outScore, uint16_t* outData = nullptr, size_t maxLen = 0);
    void clear();
    void hibernate();       // Deep sleep (10µA), waking on Button A
    void enterLightSleep(); // Pitch-black screen sleep (<1mA), wakes in <1ms on Button Back or A
    uint32_t getPersistentHighScore(uint8_t gameId);
    void updatePersistentHighScore(uint8_t gameId, uint32_t newScore);
};

extern PowerStateEngine powerEngine;
