#pragma once
#include <Arduino.h>

/**
 * ═════════════════════════════════════════════════════════════════════════
 *  Meantendo Cryptographic Execution Journal & Anti-Tamper Telemetry
 *  - Uses ESP32 Hardware Cryptographic SHA-256 Accelerators
 *  - Seeded with Hardware True Random Generator (TRNG: esp_random())
 *  - Produces verifiable high-score proof seals (e.g. MT-8B4F-29AE-C104)
 * ═════════════════════════════════════════════════════════════════════════
 */

class CryptoJournal {
public:
    uint8_t currentHash[32];
    uint32_t sessionSalt = 0;
    uint32_t eventCount = 0;
    bool sessionActive = false;

    void beginSession();
    void recordEvent(uint8_t eventType, uint32_t deltaScore, uint32_t totalScore);
    String generateScoreSeal(uint32_t finalScore);
    uint32_t getEventCount() const { return eventCount; }
};

extern CryptoJournal cryptoJournal;
