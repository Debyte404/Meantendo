// ═══════════════════════════════════════════════════════════
//  Meantendo — Main Console Entry Point
//  Boots splash → menu → game selection loop
// ═══════════════════════════════════════════════════════════

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// ── Core Modules ──────────────────────────────────────────
#include "core/Display.hpp"          // tft instance + displayInit()
#include "core/Input.hpp"            // joystick + button helpers + smart snapback
#include "core/Buzzer.hpp"           // sound feedback
#include "core/Game.hpp"             // GameDef registry
#include "core/Splash.hpp"           // debyte_logo bitmap (PROGMEM)
#include "core/Menu.hpp"             // menu draw + input handler
#include "core/Wireless.hpp"         // ESP-NOW wireless telemetry
#include "core/PowerStateEngine.hpp" // RTC state persistence & deep sleep
#include "core/RasterEngine.hpp"     // Zero-frame-buffer & dirty-span engine
#include "core/CryptoJournal.hpp"    // Hardware SHA-256 anti-tamper journal
#include "core/SelfTest.hpp"         // Automated system diagnostics
#include "core/Config.hpp"           // Version & GitHub repo settings
#include "core/CloudUpdater.hpp"     // WiFi & GitHub Cloud OTA engine

// ── Extern Storage for Game Registry ──────────────────────
GameDef* gameRegistry[MAX_GAMES];
int gameCount = 0;

// ── Game Externs ──────────────────────────────────────────
// Retro Games
extern GameDef snakeGame;
extern GameDef pongGame;
extern GameDef tetrisGame;
extern GameDef flappyGame;
extern GameDef asteroidsGame;
extern GameDef breakoutGame;
extern GameDef conwayGame;

// Visual Demos & Patent Tools
extern GameDef systemLabApp;
extern GameDef wirelessPadApp;
extern GameDef wifiUpdaterApp;
extern GameDef kaleidoscopeDemo;
extern GameDef matrixDemo;
extern GameDef mazeDemo;
extern GameDef particlesDemo;
extern GameDef fractalsDemo;
extern GameDef slimeDemo;

// ── Splash Screen ─────────────────────────────────────────
static const uint16_t LOGO_WIDTH  = 111;
static const uint16_t LOGO_HEIGHT = 111;

static void drawBitmapVertical(
    Adafruit_GFX &display,
    int16_t x, int16_t y,
    const uint8_t *bitmap,
    int16_t w, int16_t h,
    uint16_t color)
{
  for (int16_t j = 0; j < h; j++) {
    for (int16_t i = 0; i < w; i++) {
      uint8_t b = pgm_read_byte(bitmap + (i + (j / 8) * w));
      if (b & (1 << (j & 7))) {
        display.drawPixel(x + i, y + j, color);
      }
    }
  }
}

static void showSplash() {
  tft.fillScreen(ST77XX_BLACK);

  int16_t x = (tft.width()  - LOGO_WIDTH)  / 2;
  int16_t y = (tft.height() - LOGO_HEIGHT) / 2;

  // ── Fade In ──
  for (int b = 0; b <= 255; b += 10) {
    uint16_t color = tft.color565(b, b, b);
    drawBitmapVertical(tft, x, y, debyte_logo, LOGO_WIDTH, LOGO_HEIGHT, color);
    delay(10);
  }

  delay(1200);

  // ── Fade Out ──
  for (int b = 255; b >= 0; b -= 10) {
    uint16_t color = tft.color565(b, b, b);
    drawBitmapVertical(tft, x, y, debyte_logo, LOGO_WIDTH, LOGO_HEIGHT, color);
    delay(10);
  }

  tft.fillScreen(ST77XX_BLACK);
}

// ═══════════════════════════════════════════════════════════
//  SETUP — runs once on boot
// ═══════════════════════════════════════════════════════════
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== Meantendo Console v2.0 [Quad-Engine Patent Edition] ===");

  // ── Hardware Init ──
  displayInit();
  initInput();
  Buzzer::init();
  powerEngine.init();
  rasterEngine.init();
  MeantendoWireless::init();

  Serial.println("[OK] Display, Input, Buzzer, Power, Raster, Wireless initialized");

  // ── Register Games (category 0) ──
  registerGame(&snakeGame);
  registerGame(&pongGame);
  registerGame(&tetrisGame);
  registerGame(&flappyGame);
  registerGame(&asteroidsGame);
  registerGame(&breakoutGame);
  registerGame(&conwayGame);

  // ── Register Tools & Visual Demos (category 1) ──
  registerGame(&systemLabApp);
  registerGame(&wirelessPadApp);
  registerGame(&wifiUpdaterApp);
  registerGame(&kaleidoscopeDemo);
  registerGame(&matrixDemo);
  registerGame(&mazeDemo);
  registerGame(&particlesDemo);
  registerGame(&fractalsDemo);
  registerGame(&slimeDemo);

  Serial.printf("[OK] %d apps registered\n", gameCount);

  // ── Run Self-Test Diagnostics ──
  MeantendoSelfTest::runAllDiagnostics();

  // ── Splash ──
  showSplash();

  // ── Boot into Menu ──
  Buzzer::play(Buzzer::Sound::Confirm);
  initMenu();

  Serial.println("[OK] Menu active -- ready to play!");
}

// ═══════════════════════════════════════════════════════════
//  LOOP — runs forever
// ═══════════════════════════════════════════════════════════
void loop() {
  handleMenuInput();

  // Process Serial Setup Commands
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.startsWith("SETWIFI ")) {
      int spaceIdx = line.indexOf(' ', 8);
      if (spaceIdx != -1) {
        String ssid = line.substring(8, spaceIdx);
        String pass = line.substring(spaceIdx + 1);
        CloudUpdater::saveCredentials(ssid, pass);
        Serial.printf("[WIFI] Saved credentials for '%s' to NVS!\n", ssid.c_str());
      } else {
        String ssid = line.substring(8);
        CloudUpdater::saveCredentials(ssid, "");
        Serial.printf("[WIFI] Saved open network '%s' to NVS!\n", ssid.c_str());
      }
    } else if (line.equalsIgnoreCase("STATUS")) {
      Serial.printf("[STATUS] Version: %s | Free Heap: %u B | WiFi: %s\n",
                    MEANTENDO_VERSION, ESP.getFreeHeap(),
                    WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected");
    } else if (line.equalsIgnoreCase("REBOOT")) {
      Serial.println("[SYSTEM] Rebooting console...");
      ESP.restart();
    }
  }

  // Periodic Telemetry Heartbeat over Serial
  static unsigned long lastHeartbeat = 0;
  unsigned long now = millis();
  if (now - lastHeartbeat >= 2000) {
    lastHeartbeat = now;
    uint8_t btns = readButtonsBitmask();
    int x = readJoystickAnalogX();
    int y = readJoystickAnalogY();
    Serial.printf("[HEARTBEAT] Up: %4us | Stick: (X=%4d, Y=%4d) | Btns: 0x%02X | TX: %5u pkts | Free Heap: %6u B\n",
                  (unsigned int)(now / 1000), x, y, btns,
                  MeantendoWireless::getPacketsSent(),
                  ESP.getFreeHeap());
  }
}
