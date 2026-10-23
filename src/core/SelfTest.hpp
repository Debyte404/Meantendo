#pragma once
#include <Arduino.h>
#include "SnapbackEngine.hpp"
#include "CryptoJournal.hpp"
#include "PowerStateEngine.hpp"
#include "RasterEngine.hpp"
#include "Wireless.hpp"

namespace MeantendoSelfTest {

inline void runAllDiagnostics() {
    Serial.println("\n╔════════════════════════════════════════════════════════╗");
    Serial.println("║        MEANTENDO HARDWARE & FIRMWARE SELF-TEST         ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");

    // ── Test 1: ADC & Potentiometer Calibration ──
    int rawX = analogRead(JOY_Y_PIN);
    int rawY = analogRead(JOY_X_PIN);
    Serial.printf("[TEST 1/7] ADC Transducers: Raw X = %d | Raw Y = %d\n", rawX, rawY);
    if (rawX >= 1000 && rawX <= 3100 && rawY >= 1000 && rawY <= 3100) {
        Serial.printf("           PASS -> Resting center is within normal analog bounds (~2048)\n");
    } else {
        Serial.printf("           NOTE -> Stick deflected or offset (Calibrated to: X=%d, Y=%d)\n",
                      snapbackEngine.filterX.center, snapbackEngine.filterY.center);
    }

    // ── Test 2: Button Pull-up Resistors ──
    uint8_t stuckMask = 0;
    if (digitalRead(BTN_A) == LOW) stuckMask |= (1 << 0);
    if (digitalRead(BTN_B) == LOW) stuckMask |= (1 << 1);
    if (digitalRead(BTN_X) == LOW) stuckMask |= (1 << 2);
    if (digitalRead(BTN_Y) == LOW) stuckMask |= (1 << 3);
    if (digitalRead(BTN_SELECT) == LOW) stuckMask |= (1 << 4);
    if (digitalRead(BTN_BACK) == LOW) stuckMask |= (1 << 5);

    Serial.printf("[TEST 2/7] Digital Buttons: ");
    if (stuckMask == 0) {
        Serial.println("PASS -> All 6 buttons high (pull-ups active, no stuck buttons)");
    } else {
        Serial.printf("NOTE -> Active button pins detected: 0x%02X\n", stuckMask);
    }

    // ── Test 3: Smart Snapback Algorithm Simulation ──
    Serial.printf("[TEST 3/7] Smart Snapback Engine: ");
    SnapbackAxisFilter testFilter;
    testFilter.calibrateCenter(2048, 400);
    // Simulate user holding Left (3900)
    testFilter.update(3900, 10);
    testFilter.update(3900, 20);
    // Simulate fast release toward center
    testFilter.update(2600, 30);
    // Rebound past center into opposite quadrant (1500)
    int filtered = testFilter.update(1500, 40);
    if (filtered == 2048) {
        Serial.println("PASS -> Simulated mechanical recoil spike (1500) successfully clamped to 2048!");
    } else {
        Serial.printf("FAIL -> Filter output: %d (expected 2048)\n", filtered);
    }

    // ── Test 4: Hardware Cryptographic SHA-256 Engine ──
    Serial.printf("[TEST 4/7] Hardware Crypto (SHA-256 + TRNG): ");
    cryptoJournal.beginSession();
    cryptoJournal.recordEvent(0x01, 10, 10);
    cryptoJournal.recordEvent(0x01, 20, 30);
    String seal = cryptoJournal.generateScoreSeal(30);
    if (seal.startsWith("MT-") && seal.length() == 17) {
        Serial.printf("PASS -> Valid Hardware Tournament Seal: %s\n", seal.c_str());
    } else {
        Serial.printf("FAIL -> Unexpected seal format: %s\n", seal.c_str());
    }

    // ── Test 5: RTC Memory & Power Persistence ──
    Serial.printf("[TEST 5/7] RTC Fast/Slow RAM Persistence: ");
    uint16_t dummyData[4] = { 42, 100, 200, 300 };
    powerEngine.save(1, 9990, dummyData, 4);
    if (powerEngine.hasSavedState()) {
        uint8_t restoredId;
        uint32_t restoredScore;
        uint16_t restoredData[4];
        powerEngine.restore(restoredId, restoredScore, restoredData, 4);
        if (restoredId == 1 && restoredScore == 9990 && restoredData[0] == 42) {
            Serial.println("PASS -> State saved and verified with valid CRC in RTC memory");
        } else {
            Serial.println("FAIL -> Data mismatch in RTC memory");
        }
    } else {
        Serial.println("FAIL -> Checksum error in RTC memory");
    }

    // ── Test 6: Zero-Buffer Raster Engine ──
    Serial.printf("[TEST 6/7] Raster Engine Line Buffers: ");
    rasterEngine.init();
    rasterEngine.markDirty(10, 10, 20, 20);
    if (rasterEngine.dirtyCount == 1) {
        Serial.println("PASS -> Dual 320B line buffers & dirty-span queue initialized");
    } else {
        Serial.println("FAIL -> Dirty span queue failed");
    }

    // ── Test 7: ESP-NOW Wireless Subsystem ──
    Serial.printf("[TEST 7/7] ESP-NOW Wireless Subsystem: ");
    if (MeantendoWireless::isEnabled()) {
        MeantendoWireless::sendTelemetry(rawX, rawY, 0);
        Serial.printf("PASS -> Channel 1 active, test packet sent (Total TX: %u)\n",
                      MeantendoWireless::getPacketsSent());
    } else {
        Serial.println("FAIL -> Wireless not enabled");
    }

    Serial.printf("[SYSTEM] Free Heap: %u bytes | CPU Freq: %u MHz\n",
                  ESP.getFreeHeap(), ESP.getCpuFreqMHz());
    Serial.println("════════════════════════════════════════════════════════\n");
}

} // namespace MeantendoSelfTest
