#include "../core/Game.hpp"
#include "../core/Input.hpp"
#include "../core/Buzzer.hpp"
#include "../core/Config.hpp"
#include <Adafruit_ST7735.h>
#include <BleGamepad.h>
#include <NimBLEDevice.h>

extern Adafruit_ST7735 tft;

// ═══════════════════════════════════════════════════════════
//  Meantendo Bluetooth LE HID Controller
//  Identifies as standard "Meantendo Pad" on PC, Mac, Android, iOS
//  Integrated with Smart Snapback Kinematic Filter
// ═══════════════════════════════════════════════════════════

static BleGamepad bleGamepad("Meantendo Pad", "Debyte", 100);
static bool bleStarted = false;
static bool wasConnected = false;

void stopBluetoothPad() {
    NimBLEServer* pServer = NimBLEDevice::getServer();
    if (pServer) {
        // 1. Prevent NimBLE from auto-restarting advertising when disconnect fires
        pServer->advertiseOnDisconnect(false);

        // 2. Stop any active advertising immediately so host cannot rediscover/reconnect
        pServer->stopAdvertising();
        NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
        if (pAdv) {
            pAdv->stop();
        }

        // 3. Send disconnect signal to all connected peer devices (laptop, PC, phone)
        std::vector<uint16_t> peers = pServer->getPeerDevices();
        for (uint16_t handle : peers) {
            pServer->disconnect(handle, BLE_ERR_REM_USER_CONN_TERM);
        }

        // 4. Wait up to 300ms for disconnect packets to complete over the air
        unsigned long startWait = millis();
        while (pServer->getConnectedCount() > 0 && (millis() - startWait < 300)) {
            delay(10);
        }

        // 5. Ensure advertising is definitively halted
        pServer->stopAdvertising();
        if (pAdv) {
            pAdv->stop();
        }

        wasConnected = false;
        Serial.println("[BT] Gamepad disconnected, advertising completely halted");
    }
}

static void drawBlePadFrame() {
    tft.fillScreen(ST77XX_BLACK);

    // Header bar (Height 16)
    tft.fillRect(0, 0, tft.width(), 16, tft.color565(20, 25, 45));
    tft.drawFastHLine(0, 16, tft.width(), tft.color565(0, 200, 255));
    tft.setTextColor(tft.color565(0, 220, 255));
    tft.setTextSize(1);
    tft.setCursor(6, 4);
    tft.print("BLUETOOTH GAMEPAD");

    tft.setTextColor(tft.color565(120, 140, 180));
    int verLen = (int)strlen(MEANTENDO_VERSION);
    tft.setCursor(tft.width() - (verLen * 6) - 4, 4);
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
    tft.fillRect(0, tft.height() - 13, tft.width(), 13, tft.color565(15, 15, 25));
    tft.drawFastHLine(0, tft.height() - 13, tft.width(), tft.color565(50, 50, 70));
    tft.setTextColor(tft.color565(150, 160, 180));
    tft.setCursor(26, tft.height() - 9);
    tft.print("BACK: Exit Gamepad");
}

static void startBluetoothPad() {
    if (!bleStarted) {
        bleGamepad.begin();
        bleStarted = true;
    } else {
        NimBLEServer* pServer = NimBLEDevice::getServer();
        if (pServer) {
            pServer->advertiseOnDisconnect(true);
            pServer->startAdvertising();
            NimBLEAdvertising* pAdv = NimBLEDevice::getAdvertising();
            if (pAdv) {
                pAdv->start();
            }
        }
    }

    wasConnected = false;
    drawBlePadFrame();
    Buzzer::play(Buzzer::Sound::Confirm);
}

static void loopBluetoothPad() {
    bool connected = bleGamepad.isConnected();

    // Connection state change audio feedback
    if (connected != wasConnected) {
        wasConnected = connected;
        Buzzer::play(connected ? Buzzer::Sound::Confirm : Buzzer::Sound::Click);
    }

    // Status Banner at y=84 (Height 16, centered text)
    tft.fillRect(0, 84, tft.width(), 16, ST77XX_BLACK);
    if (connected) {
        tft.fillRoundRect(8, 85, 144, 14, 2, tft.color565(0, 150, 70));
        tft.setTextColor(ST77XX_WHITE);
        tft.setCursor(29, 88);
        tft.print("CONNECTED TO HOST");
    } else {
        bool blink = ((millis() / 400) % 2 == 0);
        tft.drawRoundRect(8, 85, 144, 14, 2, blink ? tft.color565(0, 200, 255) : tft.color565(50, 60, 80));
        tft.setTextColor(blink ? tft.color565(0, 220, 255) : tft.color565(100, 120, 150));
        tft.setCursor(41, 88);
        tft.print("READY TO PAIR");
    }

    // Sub-text at y=102 (Height 11, centered text)
    tft.fillRect(0, 102, tft.width(), 11, ST77XX_BLACK);
    tft.setTextColor(tft.color565(130, 150, 180));
    if (connected) {
        tft.setCursor(14, 103);
        tft.print("Zero-Lag Snapback Active");
    } else {
        tft.setCursor(17, 103);
        tft.print("Search: 'Meantendo Pad'");
    }

    // Read analog stick (Inverted X PCB orientation + Snapback filtered)
    int rawX = 4095 - analogRead(JOY_Y_PIN);
    int rawY = analogRead(JOY_X_PIN);
    int filtX, filtY;
    snapbackEngine.filter(rawX, rawY, filtX, filtY);

    // Map stick to 46x46 radar box
    int dotX = map(filtX, 0, 4095, 12, 50);
    int dotY = map(filtY, 0, 4095, 39, 77);

    // Refresh crosshair interior safely without eroding border
    tft.fillRect(9, 36, 44, 44, ST77XX_BLACK);
    tft.drawRoundRect(8, 35, 46, 46, 3, tft.color565(60, 70, 95));
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
