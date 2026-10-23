#include "CryptoJournal.hpp"
#include <mbedtls/sha256.h>
#include <mbedtls/version.h>

CryptoJournal cryptoJournal;

static inline void safe_sha256_starts(mbedtls_sha256_context *ctx, int is224) {
#if defined(MBEDTLS_VERSION_NUMBER) && (MBEDTLS_VERSION_NUMBER >= 0x03000000)
    mbedtls_sha256_starts(ctx, is224);
#else
    mbedtls_sha256_starts_ret(ctx, is224);
#endif
}

static inline void safe_sha256_update(mbedtls_sha256_context *ctx, const unsigned char *input, size_t ilen) {
#if defined(MBEDTLS_VERSION_NUMBER) && (MBEDTLS_VERSION_NUMBER >= 0x03000000)
    mbedtls_sha256_update(ctx, input, ilen);
#else
    mbedtls_sha256_update_ret(ctx, input, ilen);
#endif
}

static inline void safe_sha256_finish(mbedtls_sha256_context *ctx, unsigned char output[32]) {
#if defined(MBEDTLS_VERSION_NUMBER) && (MBEDTLS_VERSION_NUMBER >= 0x03000000)
    mbedtls_sha256_finish(ctx, output);
#else
    mbedtls_sha256_finish_ret(ctx, output);
#endif
}

void CryptoJournal::beginSession() {
    sessionSalt = esp_random();
    eventCount = 0;
    sessionActive = true;

    // Genesis Block
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    safe_sha256_starts(&ctx, 0); // 0 = SHA-256

    uint32_t genesisData[3] = { sessionSalt, (uint32_t)micros(), 0x54454E44 }; // "TEND"
    safe_sha256_update(&ctx, reinterpret_cast<const uint8_t*>(genesisData), sizeof(genesisData));
    safe_sha256_finish(&ctx, currentHash);
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
    safe_sha256_starts(&ctx, 0);
    safe_sha256_update(&ctx, reinterpret_cast<const uint8_t*>(&payload), sizeof(payload));
    safe_sha256_finish(&ctx, currentHash);
    mbedtls_sha256_free(&ctx);
}

String CryptoJournal::generateScoreSeal(uint32_t finalScore) {
    if (!sessionActive) beginSession();

    // Final sealing block
    recordEvent(0xFF, 0, finalScore);
    sessionActive = false;

    char sealBuf[17];
    for (int i = 0; i < 8; i++) {
        sprintf(sealBuf + (i * 2), "%02X", currentHash[i]);
    }
    sealBuf[16] = '\0';

    Serial.printf("[CRYPTO] Score Seal generated for %lu points (%lu events): %s\n",
                  (unsigned long)finalScore, (unsigned long)eventCount, sealBuf);
    return String(sealBuf);
}
