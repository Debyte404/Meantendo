#include "../core/Game.hpp"
#include "../core/Input.hpp"
#include "../core/Wireless.hpp"
#include <Adafruit_ST7735.h>

extern Adafruit_ST7735 tft;

// ═══════════════════════════════════════════════════════════
//  Meantendo Wireless Controller Mode
//  Acts as a live wireless gamepad streaming to ESP32-S2 / Web
// ═══════════════════════════════════════════════════════════

static void startWirelessPad() {
    tft.fillScreen(ST77XX_BLACK);

    // Title bar
    tft.fillRect(0, 0, tft.width(), 18, tft.color565(20, 20, 35));
    tft.drawFastHLine(0, 18, tft.width(), tft.color565(0, 200, 255));
    tft.setTextColor(tft.color565(0, 220, 255));
    tft.setTextSize(1);
    tft.setCursor(18, 5);
    tft.print("WIRELESS GAMEPAD");

    // Static Labels
    tft.setTextColor(tft.color565(140, 140, 160));
    tft.setCursor(10, 24);
    tft.print("RF: ESP-NOW (Ch 1)");

    // Joystick Boundary Box
    tft.drawRoundRect(10, 36, 50, 50, 6, tft.color565(60, 60, 90));
    tft.drawFastHLine(10, 61, 50, tft.color565(40, 40, 60));
    tft.drawFastVLine(35, 36, 50, tft.color565(40, 40, 60));

    // Button Labels
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(75, 42); tft.print("A:");
    tft.setCursor(115, 42); tft.print("B:");
    tft.setCursor(75, 62); tft.print("X:");
    tft.setCursor(115, 62); tft.print("Y:");

    // Footer
    tft.fillRect(0, tft.height() - 14, tft.width(), 14, tft.color565(20, 20, 30));
    tft.drawFastHLine(0, tft.height() - 14, tft.width(), tft.color565(50, 50, 70));
    tft.setTextColor(tft.color565(180, 180, 180));
    tft.setCursor(15, tft.height() - 10);
    tft.print("Press BACK to exit");
}

static void loopWirelessPad() {
    static int lastX = 35;
    static int lastY = 61;
    static uint32_t lastPkt = 0;

    // Read analog coordinates
    int rawX = readJoystickAnalogX();
    int rawY = readJoystickAnalogY();

    // Map to stick box (center 35, 61, radius 20)
    int dotX = map(rawX, 0, 4095, 14, 56);
    int dotY = map(rawY, 0, 4095, 40, 82);

    // Redraw stick cursor if moved
    if (dotX != lastX || dotY != lastY) {
        // Clear interior safely
        tft.fillRect(11, 37, 48, 48, ST77XX_BLACK);
        // Redraw box boundary & crosshair lines cleanly
        tft.drawRoundRect(10, 36, 50, 50, 6, tft.color565(60, 60, 90));
        tft.drawFastHLine(10, 61, 50, tft.color565(40, 40, 60));
        tft.drawFastVLine(35, 36, 50, tft.color565(40, 40, 60));

        tft.fillCircle(dotX, dotY, 3, tft.color565(255, 200, 0));
        lastX = dotX;
        lastY = dotY;
    }

    // Update buttons
    auto drawBtnState = [](int x, int y, bool pressed, const char* label, uint16_t col) {
        if (pressed) {
            tft.fillRoundRect(x, y, 22, 14, 3, col);
            tft.setTextColor(ST77XX_BLACK);
        } else {
            tft.fillRoundRect(x, y, 22, 14, 3, tft.color565(30, 30, 45));
            tft.drawRoundRect(x, y, 22, 14, 3, tft.color565(70, 70, 90));
            tft.setTextColor(ST77XX_WHITE);
        }
        tft.setCursor(x + 7, y + 3);
        tft.print(label);
    };

    drawBtnState(88, 38, aPressed(), "A", tft.color565(0, 255, 120));
    drawBtnState(128, 38, bPressed(), "B", tft.color565(255, 80, 80));
    drawBtnState(88, 58, xPressed(), "X", tft.color565(80, 160, 255));
    drawBtnState(128, 58, yPressed(), "Y", tft.color565(255, 220, 0));

    // Packet counter & coordinate readout
    uint32_t pkts = MeantendoWireless::getPacketsSent();
    if (pkts != lastPkt) {
        lastPkt = pkts;
        tft.fillRect(10, 92, tft.width() - 20, 20, ST77XX_BLACK);
        tft.setTextColor(tft.color565(0, 255, 180));
        tft.setTextSize(1);
        tft.setCursor(10, 94);
        tft.printf("TX: %u pkts", pkts);

        tft.setTextColor(tft.color565(160, 160, 180));
        tft.setCursor(10, 104);
        tft.printf("X:%4d  Y:%4d", rawX, rawY);
    }

    delay(20);
}

GameDef wirelessPadApp = { "Wireless Pad", startWirelessPad, loopWirelessPad, 1 };
