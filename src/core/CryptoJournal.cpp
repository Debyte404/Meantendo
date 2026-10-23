#include "CryptoJournal.hpp"
#include <mbedtls/sha256.h>

CryptoJournal cryptoJournal;

void CryptoJournal::beginSession() {
    sessionSalt = esp_random();
    eventCount = 0;
    sessionActive = true;

    // Genesis Block
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts_ret(&ctx, 0); // 0 = SHA-256

    uint32_t genesisData[3] = { sessionSalt, (uint32_t)micros(), 0x54454E44 }; // "TEND"
    mbedtls_sha256_update_ret(&ctx, reinterpret_cast<const uint8_t*>(genesisData), sizeof(genesisData));
    mbedtls_sha256_finish_ret(&ctx, currentHash);
    mbedtls_sha256_free(&ctx);

    Serial.printf("[CRYPTO] Session initialized with Hardware TRNG Salt: 0x%08X\n", sessionSalt);
}

void CryptoJournal::recordEvent(uint8_t eventType, uint32_t deltaScore, uint32_t totalScore) {
    if (!sessionActive) beginSession();

    eventCount++;
    unsigned long now = millis();

    struct __attribute__((packed)) EventPayload {
        uint8_t prevHash[32];
        uint32_t eventIndex;
        uint32_t timestamp;
        uint32_t delta;
        uint32_t currentTotal;
        uint8_t type;
    } payload;

    memcpy(payload.prevHash, currentHash, 32);
    payload.eventIndex = eventCount;
    payload.timestamp = now;
    payload.delta = deltaScore;
    payload.currentTotal = totalScore;
    payload.type = eventType;

    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts_ret(&ctx, 0);
    mbedtls_sha256_update_ret(&ctx, reinterpret_cast<const uint8_t*>(&payload), sizeof(payload));
    mbedtls_sha256_finish_ret(&ctx, currentHash);
    mbedtls_sha256_free(&ctx);
}

String CryptoJournal::generateScoreSeal(uint32_t finalScore) {
    if (!sessionActive) beginSession();

    // Final sealing block
    recordEvent(0xFF, 0, finalScore);
    sessionActive = false;

    // Produce compact 16-character hex certificate: MT-XXXX-YYYY-ZZZZ
    char sealBuf[24];
    snprintf(sealBuf, sizeof(sealBuf), "MT-%02X%02X-%02X%02X-%02X%02X",
             currentHash[0], currentHash[1],
             currentHash[2], currentHash[3],
             currentHash[4], currentHash[5]);

    Serial.printf("[CRYPTO] Score Seal generated for %u points (%u events): %s\n",
                  finalScore, eventCount, sealBuf);
    return String(sealBuf);
}
