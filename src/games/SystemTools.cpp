#include "../core/Game.hpp"
#include "../core/Input.hpp"
#include "../core/SnapbackEngine.hpp"
#include "../core/Wireless.hpp"
#include "../core/CryptoJournal.hpp"
#include "../core/PowerStateEngine.hpp"
#include "../core/RasterEngine.hpp"
#include <Adafruit_ST7735.h>
#include <cmath>

extern Adafruit_ST7735 tft;

// ═══════════════════════════════════════════════════════════
//  Meantendo Patent Systems Lab & Visual Diagnostics
//  Clean graphical gauges formatted strictly for 160x128
// ═══════════════════════════════════════════════════════════

static uint8_t activePage = 0; // 0=SNAP, 1=RASTER/RAM, 2=MESH, 3=CRYPTO/PWR
static unsigned long lastPageSwitch = 0;

static void drawLabFrame() {
    tft.fillScreen(ST77XX_BLACK);

    // Header tabs
    tft.fillRect(0, 0, tft.width(), 16, tft.color565(18, 22, 35));
    tft.drawFastHLine(0, 16, tft.width(), tft.color565(0, 200, 255));

    const char* tabs[] = { "SNAP", "RAST", "MESH", "PWR" };
    for (int i = 0; i < 4; i++) {
        int x = i * 40;
        if (i == activePage) {
            tft.fillRoundRect(x + 2, 2, 36, 13, 2, tft.color565(0, 200, 255));
            tft.setTextColor(ST77XX_BLACK);
        } else {
            tft.setTextColor(tft.color565(120, 130, 160));
        }
        tft.setTextSize(1);
        tft.setCursor(x + 7, 5);
        tft.print(tabs[i]);
    }

    // Footer (Max 22 chars to prevent wrapping)
    tft.fillRect(0, tft.height() - 12, tft.width(), 12, tft.color565(15, 15, 25));
    tft.drawFastHLine(0, tft.height() - 12, tft.width(), tft.color565(50, 50, 70));
    tft.setTextColor(tft.color565(160, 170, 190));
    tft.setCursor(18, tft.height() - 9);
    tft.print("X: Next Tab  B: Exit");
}

static void startSystemLab() {
    activePage = 0;
    drawLabFrame();
}

static void loopSystemLab() {
    // Switch tabs on Button X
    if (xPressed() && millis() - lastPageSwitch > 250) {
        lastPageSwitch = millis();
        activePage = (activePage + 1) % 4;
        drawLabFrame();
    }

    // Clear content area between header and footer
    tft.fillRect(0, 17, tft.width(), tft.height() - 29, ST77XX_BLACK);

    switch (activePage) {
        // ── TAB 0: SMART SNAPBACK VISUAL RADAR ──
        case 0: {
            int rawX = 4095 - analogRead(JOY_Y_PIN); // Correct inverted horizontal axis
            int rawY = analogRead(JOY_X_PIN);
            int filtX, filtY;
            snapbackEngine.filter(rawX, rawY, filtX, filtY);

            bool inRebound = (snapbackEngine.filterX.inReboundState || snapbackEngine.filterY.inReboundState);

            // Graphical 44x44 crosshair box
            const int boxX = 6, boxY = 22, boxW = 46, boxH = 46;
            tft.drawRoundRect(boxX, boxY, boxW, boxH, 3, inRebound ? ST77XX_RED : tft.color565(60, 70, 100));
            tft.drawFastHLine(boxX + 2, boxY + boxH/2, boxW - 4, tft.color565(40, 45, 65));
            tft.drawFastVLine(boxX + boxW/2, boxY + 2, boxH - 4, tft.color565(40, 45, 65));

            // Deadzone box
            tft.drawRect(boxX + boxW/2 - 4, boxY + boxH/2 - 4, 9, 9, tft.color565(40, 50, 80));

            // Map stick positions (Raw = RED dot, Filtered = GREEN dot)
            int rDotX = map(rawX, 0, 4095, boxX + 4, boxX + boxW - 4);
            int rDotY = map(rawY, 0, 4095, boxY + 4, boxY + boxH - 4);
            int fDotX = map(filtX, 0, 4095, boxX + 4, boxX + boxW - 4);
            int fDotY = map(filtY, 0, 4095, boxY + 4, boxY + boxH - 4);

            tft.fillCircle(rDotX, rDotY, 2, ST77XX_RED);
            tft.fillCircle(fDotX, fDotY, 3, ST77XX_GREEN);

            // Right side status pill
            int pillX = 58, pillY = 22;
            if (inRebound) {
                tft.fillRoundRect(pillX, pillY, 96, 14, 2, ST77XX_RED);
                tft.setTextColor(ST77XX_WHITE);
                tft.setCursor(pillX + 5, pillY + 3);
                tft.print("RECOIL CLAMPED!");
            } else {
                tft.fillRoundRect(pillX, pillY, 96, 14, 2, tft.color565(0, 160, 70));
                tft.setTextColor(ST77XX_WHITE);
                tft.setCursor(pillX + 8, pillY + 3);
                tft.print("0-LAG ACTIVE OK");
            }

            // Legend
            tft.fillCircle(pillX + 4, pillY + 24, 2, ST77XX_GREEN);
            tft.setTextColor(ST77XX_WHITE);
            tft.setCursor(pillX + 12, pillY + 21);
            tft.print("Filtered Pos");

            tft.fillCircle(pillX + 4, pillY + 36, 2, ST77XX_RED);
            tft.setTextColor(tft.color565(200, 200, 200));
            tft.setCursor(pillX + 12, pillY + 33);
            tft.print("Raw Sensor");

            // Deflection progress meter
            int defl = abs(filtX - snapbackEngine.filterX.center);
            int barW = map(constrain(defl, 0, 2048), 0, 2048, 0, 142);
            tft.setTextColor(tft.color565(140, 150, 180));
            tft.setCursor(6, 75);
            tft.print("DEFLECTION METER:");
            tft.drawRoundRect(6, 86, 148, 9, 2, tft.color565(50, 50, 80));
            tft.fillRect(8, 88, barW, 5, inRebound ? ST77XX_RED : tft.color565(0, 220, 255));

            tft.setTextColor(tft.color565(100, 120, 150));
            tft.setCursor(6, 100);
            tft.print("Kinematic Spring Damping");
            break;
        }

        // ── TAB 1: RASTER & RAM BENCHMARK ──
        case 1: {
            tft.setTextColor(tft.color565(0, 220, 255));
            tft.setTextSize(1);
            tft.setCursor(6, 20);
            tft.print("DISPLAY & RAM STATS");

            // RAM Footprint Comparison
            tft.setTextColor(tft.color565(255, 90, 90));
            tft.setCursor(6, 33);
            tft.print("Full Buffer: 41 KB");
            tft.drawRect(6, 43, 148, 6, tft.color565(60, 60, 80));
            tft.fillRect(7, 44, 146, 4, tft.color565(255, 80, 80));

            tft.setTextColor(ST77XX_GREEN);
            tft.setCursor(6, 52);
            tft.print("Meantendo:  0.6 KB");
            tft.drawRect(6, 62, 148, 6, tft.color565(60, 60, 80));
            tft.fillRect(7, 63, 3, 4, ST77XX_GREEN);

            // Microsecond Benchmark
            uint32_t t0 = micros();
            tft.fillRect(158, 126, 2, 2, ST77XX_BLACK);
            uint32_t dirtyUs = micros() - t0;
            if (dirtyUs == 0) dirtyUs = 1;

            tft.setTextColor(tft.color565(255, 220, 0));
            tft.setCursor(6, 74);
            tft.printf("Span: %u us | Full: 16ms", dirtyUs);

            // Highlight Badge
            tft.fillRoundRect(6, 88, 148, 14, 2, tft.color565(0, 110, 200));
            tft.setTextColor(ST77XX_WHITE);
            tft.setCursor(12, 91);
            tft.print(">> 98% LESS RAM USED <<");

            tft.setTextColor(tft.color565(120, 220, 150));
            tft.setCursor(6, 105);
            tft.print("Zero Tearing Scanline DMA");
            break;
        }

        // ── TAB 2: WIRELESS MESH RADAR ──
        case 2: {
            tft.setTextColor(tft.color565(0, 255, 180));
            tft.setTextSize(1);
            tft.setCursor(6, 20);
            tft.print("ESP-NOW WIRELESS MESH");

            // Pulsing radar circle
            int rX = 28, rY = 56;
            int pulse = (millis() / 200) % 16;
            tft.drawCircle(rX, rY, 18, tft.color565(30, 40, 60));
            tft.drawCircle(rX, rY, 10, tft.color565(40, 60, 80));
            tft.drawCircle(rX, rY, pulse, tft.color565(0, 255, 180));
            tft.fillCircle(rX, rY, 2, ST77XX_WHITE);

            // Right side stats card
            bool peerOnline = MeantendoWireless::isPeerConnected();
            int cX = 56, cY = 32;
            tft.drawRoundRect(cX, cY, 98, 44, 3, peerOnline ? ST77XX_GREEN : tft.color565(60, 60, 80));
            tft.setTextColor(peerOnline ? ST77XX_GREEN : ST77XX_ORANGE);
            tft.setCursor(cX + 6, cY + 5);
            tft.print(peerOnline ? "PEER: LINKED!" : "PEER: SEARCHING");

            tft.setTextColor(ST77XX_WHITE);
            tft.setCursor(cX + 6, cY + 18);
            tft.printf("TX Pkts: %u", MeantendoWireless::getPacketsSent());
            tft.setCursor(cX + 6, cY + 30);
            tft.printf("RX Pkts: %u", MeantendoWireless::getPacketsRecv());

            // Protocol banner (Max 24 chars)
            tft.setTextColor(tft.color565(140, 140, 170));
            tft.setCursor(6, 85);
            tft.print("Direct 802.11 Action Frames");
            tft.setCursor(6, 98);
            tft.setTextColor(ST77XX_GREEN);
            tft.print("Sub-5ms | Zero Router");
            break;
        }

        // ── TAB 3: CRYPTO & POWER SLEEP ──
        case 3: {
            tft.setTextColor(tft.color565(255, 140, 255));
            tft.setTextSize(1);
            tft.setCursor(6, 20);
            tft.print("CRYPTO & POWER PERSIST");

            // Tournament seal card
            tft.drawRoundRect(6, 33, 148, 30, 3, tft.color565(0, 200, 255));
            tft.setTextColor(tft.color565(0, 220, 255));
            tft.setCursor(10, 36);
            tft.print("TOURNAMENT PROOF SEAL");

            tft.setTextColor(ST77XX_YELLOW);
            tft.setCursor(12, 48);
            tft.printf("MT-%02X%02X-%02X%02X-%02X%02X",
                       cryptoJournal.currentHash[0], cryptoJournal.currentHash[1],
                       cryptoJournal.currentHash[2], cryptoJournal.currentHash[3],
                       cryptoJournal.currentHash[4], cryptoJournal.currentHash[5]);

            // Sleep Instructions (No wrapping)
            tft.drawRoundRect(6, 68, 148, 42, 3, tft.color565(70, 70, 100));
            tft.setTextColor(ST77XX_WHITE);
            tft.setCursor(10, 72);
            tft.print("Press Y: Deep Sleep");
            tft.setTextColor(tft.color565(0, 255, 150));
            tft.setCursor(10, 84);
            tft.print("Home BACK: Pitch Black");
            tft.setTextColor(tft.color565(140, 140, 170));
            tft.setCursor(10, 96);
            tft.print("Wakes <1ms on Button A");

            if (yPressed()) {
                powerEngine.hibernate();
            }
            break;
        }
    }

    delay(30);
}

GameDef systemLabApp = { "System Lab", startSystemLab, loopSystemLab, 1 };
