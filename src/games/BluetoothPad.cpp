#include "../core/Game.hpp"
#include "../core/Input.hpp"
#include "../core/Buzzer.hpp"
#include "../core/Config.hpp"
#include <Adafruit_ST7735.h>
#include <BleGamepad.h>

extern Adafruit_ST7735 tft;

// ═══════════════════════════════════════════════════════════
//  Meantendo Bluetooth LE HID Controller
//  Identifies as standard "Meantendo Pad" on PC, Mac, Android, iOS
//  Integrated with Smart Snapback Kinematic Filter
// ═══════════════════════════════════════════════════════════

static BleGamepad bleGamepad("Meantendo Pad", "Debyte", 100);
static bool bleActive = false;
static bool wasConnected = false;
static unsigned long entryTime = 0;

static void drawBlePadFrame() {
    tft.fillScreen(ST77XX_BLACK);

    // Header bar
    tft.fillRect(0, 0, tft.width(), 16, tft.color565(20, 25, 45));
    tft.drawFastHLine(0, 16, tft.width(), tft.color565(0, 200, 255));
    tft.setTextColor(tft.color565(0, 220, 255));
    tft.setTextSize(1);
    tft.setCursor(6, 4);
    tft.print("BLUETOOTH GAMEPAD");

    tft.setTextColor(tft.color565(120, 140, 180));
    tft.setCursor(120, 4);
    tft.print(MEANTENDO_VERSION);

    // Device Name Tag
    tft.setTextColor(tft.color565(200, 210, 230));
    tft.setCursor(8, 22);
    tft.print("ID: 'Meantendo Pad'");

    // Joystick Radar Box (x: 8, y: 35, w: 46, h: 46)
    tft.drawRoundRect(8, 35, 46, 46, 3, tft.color565(60, 70, 95));
    tft.drawFastHLine(10, 58, 42, tft.color565(35, 42, 60));
    tft.drawFastVLine(31, 37, 42, tft.color565(35, 42, 60));

    // Button Matrix Cards
    int bx = 62, by = 35;
    tft.drawRoundRect(bx, by, 90, 46, 3, tft.color565(40, 45, 65));
    tft.setTextColor(tft.color565(160, 170, 190));
    tft.setCursor(bx + 6, by + 5);
    tft.print("HID BUTTONS:");

    // Footer
    tft.fillRect(0, tft.height() - 12, tft.width(), 12, tft.color565(15, 15, 25));
    tft.drawFastHLine(0, tft.height() - 12, tft.width(), tft.color565(50, 50, 70));
    tft.setTextColor(tft.color565(150, 160, 180));
    tft.setCursor(12, tft.height() - 9);
    tft.print("Press BACK to Disconnect");
}

static void startBluetoothPad() {
    bleActive = true;
    wasConnected = false;
    entryTime = millis();

    drawBlePadFrame();

    // Start BLE Gamepad advertising
    bleGamepad.begin();
    Buzzer::play(Buzzer::Sound::Confirm);
}

static void loopBluetoothPad() {
    bool connected = bleGamepad.isConnected();

    // Connection state change audio/visual feedback
    if (connected != wasConnected) {
        wasConnected = connected;
        if (connected) {
            Buzzer::play(Buzzer::Sound::Confirm);
        }
    }

    // Status Banner at y=85
    tft.fillRect(6, 85, 148, 16, tft.color565(10, 12, 20));
    if (connected) {
        tft.fillRoundRect(8, 86, 144, 14, 2, tft.color565(0, 150, 70));
        tft.setTextColor(ST77XX_WHITE);
        tft.setCursor(18, 89);
        tft.print("PAIRED: PC / MOBILE OK");
    } else {
        bool blink = ((millis() / 400) % 2 == 0);
        tft.drawRoundRect(8, 86, 144, 14, 2, blink ? tft.color565(0, 200, 255) : tft.color565(50, 60, 80));
        tft.setTextColor(blink ? tft.color565(0, 220, 255) : tft.color565(100, 120, 150));
        tft.setCursor(14, 89);
        tft.print("PAIR IN BLUETOOTH SETTINGS");
    }

    // Sub-text at y=103
    tft.fillRect(6, 103, 148, 11, ST77XX_BLACK);
    tft.setTextColor(tft.color565(120, 140, 170));
    tft.setCursor(8, 104);
    if (connected) {
        tft.print("Smart Snapback Active (0 Lag)");
    } else {
        tft.print("Select 'Meantendo Pad' to connect");
    }

    // Read analog stick (Inverted X PCB orientation + Snapback filtered)
    int rawX = 4095 - analogRead(JOY_Y_PIN);
    int rawY = analogRead(JOY_X_PIN);
    int filtX, filtY;
    snapbackEngine.filter(rawX, rawY, filtX, filtY);

    // Map stick to 46x46 radar box
    int dotX = map(filtX, 0, 4095, 12, 50);
    int dotY = map(filtY, 0, 4095, 39, 77);

    // Refresh crosshair interior
    tft.fillRect(10, 37, 42, 42, ST77XX_BLACK);
    tft.drawFastHLine(10, 58, 42, tft.color565(35, 42, 60));
    tft.drawFastVLine(31, 37, 42, tft.color565(35, 42, 60));
    tft.fillCircle(dotX, dotY, 2, connected ? ST77XX_GREEN : tft.color565(0, 220, 255));

    // Read buttons
    bool btnA = (digitalRead(BTN_A) == LOW);
    bool btnB = (digitalRead(BTN_B) == LOW);
    bool btnX = (digitalRead(BTN_X) == LOW);
    bool btnY = (digitalRead(BTN_Y) == LOW);
    bool btnSel = (digitalRead(BTN_SELECT) == LOW);

    // Visual button indicators in card
    int bx = 62, by = 35;
    auto drawBtnState = [&](int x, int y, const char* name, bool pressed) {
        if (pressed) {
            tft.fillRect(x, y, 18, 10, tft.color565(255, 220, 0));
            tft.setTextColor(ST77XX_BLACK);
        } else {
            tft.fillRect(x, y, 18, 10, tft.color565(25, 30, 45));
            tft.drawRect(x, y, 18, 10, tft.color565(45, 55, 80));
            tft.setTextColor(tft.color565(180, 190, 210));
        }
        tft.setCursor(x + 3, y + 2);
        tft.print(name);
    };

    drawBtnState(bx + 8,  by + 18, "A", btnA);
    drawBtnState(bx + 30, by + 18, "B", btnB);
    drawBtnState(bx + 52, by + 18, "X", btnX);
    drawBtnState(bx + 74, by + 18, "Y", btnY);
    drawBtnState(bx + 8,  by + 31, "SEL", btnSel);

    // Transmit BLE Gamepad Report
    if (connected) {
        // Map 0..4095 to signed 16-bit range (-32767..32767)
        int16_t joyX = (int16_t)map(filtX, 0, 4095, -32767, 32767);
        int16_t joyY = (int16_t)map(filtY, 0, 4095, -32767, 32767);

        bleGamepad.setAxes(joyX, joyY);

        if (btnA) bleGamepad.press(BUTTON_1); else bleGamepad.release(BUTTON_1);
        if (btnB) bleGamepad.press(BUTTON_2); else bleGamepad.release(BUTTON_2);
        if (btnX) bleGamepad.press(BUTTON_3); else bleGamepad.release(BUTTON_3);
        if (btnY) bleGamepad.press(BUTTON_4); else bleGamepad.release(BUTTON_4);
        if (btnSel) bleGamepad.press(BUTTON_7); else bleGamepad.release(BUTTON_7);

        bleGamepad.sendReport();
    }

    delay(20);
}

GameDef bluetoothPadApp = {
    "Bluetooth Pad",
    startBluetoothPad,
    loopBluetoothPad,
    1  // Category 1: Tools & Hardware
};
