#include "PowerStateEngine.hpp"
#include <Adafruit_ST7735.h>

extern Adafruit_ST7735 tft;

// Preserved across deep sleep, watchdog resets, and brownouts
RTC_DATA_ATTR static MeantendoSavedState rtcSavedState;
RTC_DATA_ATTR static uint32_t rtcHighScores[20] = {0};

PowerStateEngine powerEngine;

static uint16_t computeChecksum(const MeantendoSavedState &s) {
    const uint8_t *p = reinterpret_cast<const uint8_t*>(&s);
    size_t len = offsetof(MeantendoSavedState, checksum);
    uint16_t sum = 0x55AA;
    for (size_t i = 0; i < len; i++) {
        sum = (sum << 1) ^ p[i];
    }
    return sum;
}

void PowerStateEngine::init() {
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause == ESP_SLEEP_WAKEUP_EXT0) {
        Serial.println("[POWER] Woke from Deep Sleep via Button A!");
    } else if (cause == ESP_SLEEP_WAKEUP_GPIO) {
        Serial.println("[POWER] Woke from Light Sleep via GPIO!");
    }
}

bool PowerStateEngine::hasSavedState() {
    if (rtcSavedState.magic != STATE_MAGIC) return false;
    return (rtcSavedState.checksum == computeChecksum(rtcSavedState));
}

bool PowerStateEngine::save(uint8_t gameId, uint32_t score, const uint16_t* extraData, size_t extraLen) {
    rtcSavedState.magic = STATE_MAGIC;
    rtcSavedState.activeGameId = gameId;
    rtcSavedState.score = score;
    rtcSavedState.saveTimestamp = millis();

    memset(rtcSavedState.gameData, 0, sizeof(rtcSavedState.gameData));
    if (extraData && extraLen > 0) {
        size_t count = min(extraLen, (size_t)32);
        memcpy(rtcSavedState.gameData, extraData, count * sizeof(uint16_t));
    }

    if (gameId < 20 && score > rtcHighScores[gameId]) {
        rtcHighScores[gameId] = score;
    }
    rtcSavedState.highScore = (gameId < 20) ? rtcHighScores[gameId] : score;
    rtcSavedState.checksum = computeChecksum(rtcSavedState);

    Serial.printf("[POWER] State saved to RTC memory for Game #%u (Score: %u)\n", gameId, score);
    return true;
}

bool PowerStateEngine::restore(uint8_t &outGameId, uint32_t &outScore, uint16_t* outData, size_t maxLen) {
    if (!hasSavedState()) return false;

    outGameId = rtcSavedState.activeGameId;
    outScore = rtcSavedState.score;

    if (outData && maxLen > 0) {
        size_t count = min(maxLen, (size_t)32);
        memcpy(outData, rtcSavedState.gameData, count * sizeof(uint16_t));
    }

    Serial.printf("[POWER] Restored state from RTC: Game #%u | Score: %u\n", outGameId, outScore);
    return true;
}

void PowerStateEngine::clear() {
    rtcSavedState.magic = 0;
    rtcSavedState.checksum = 0;
}

uint32_t PowerStateEngine::getPersistentHighScore(uint8_t gameId) {
    if (gameId < 20) return rtcHighScores[gameId];
    return 0;
}

void PowerStateEngine::updatePersistentHighScore(uint8_t gameId, uint32_t newScore) {
    if (gameId < 20 && newScore > rtcHighScores[gameId]) {
        rtcHighScores[gameId] = newScore;
    }
}

void PowerStateEngine::hibernate() {
    Serial.println("[POWER] Entering 10µA Ultra-Low-Power Deep Sleep...");
    tft.fillScreen(ST77XX_BLACK);
    delay(50);

    // Configure Button A (GPIO 32) as external wakeup source (active LOW)
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_32, 0);

    // Deep sleep powers down CPU, RF, high-speed RAM; RTC Fast/Slow memory stays powered
    esp_deep_sleep_start();
}

void PowerStateEngine::enterLightSleep() {
    Serial.println("[POWER] Entering Pitch-Black Light Sleep Mode (<1mA)...");

    // 1. Draw solid pitch black across the panel
    tft.fillScreen(ST77XX_BLACK);
    delay(30);

    // 2. Wait until buttons are physically released
    while (digitalRead(25) == LOW || digitalRead(32) == LOW) {
        delay(10);
    }
    delay(60);

    // 3. Configure GPIO wakeup (GPIO 25 is BTN_BACK, GPIO 32 is BTN_A)
    gpio_wakeup_enable(GPIO_NUM_25, GPIO_INTR_LOW_LEVEL);
    gpio_wakeup_enable(GPIO_NUM_32, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();

    // 4. Enter light sleep
    // ST7735 controller internal RAM holds the black pixels without CPU intervention
    // The screen stays completely pitch black with zero white flash
    esp_light_sleep_start();

    // 5. Wakeup debounce: wait for button release
    while (digitalRead(25) == LOW || digitalRead(32) == LOW) {
        delay(10);
    }
    delay(40);

    Serial.println("[POWER] Woke from Light Sleep in <1ms!");
}
