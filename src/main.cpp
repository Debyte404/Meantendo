// ═══════════════════════════════════════════════════════════
//  Meantendo — Main Console Entry Point
//  Boots splash → menu → game selection loop
// ═══════════════════════════════════════════════════════════

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// ── Core Modules ──────────────────────────────────────────
#include "core/Display.hpp"    // tft instance + displayInit()
#include "core/Input.hpp"      // joystick + button helpers
#include "core/Buzzer.hpp"     // sound feedback
#include "core/Game.hpp"       // GameDef registry
#include "core/Splash.hpp"     // debyte_logo bitmap (PROGMEM)
#include "core/Menu.hpp"       // menu draw + input handler

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

// Visual Demos
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
  Serial.println("\n=== Meantendo Console v1.0 ===");

  // ── Hardware Init ──
  displayInit();
  initInput();
  Buzzer::init();

  Serial.println("[OK] Display, Input, Buzzer initialized");

  // ── Register Games (category 0) ──
  registerGame(&snakeGame);
  registerGame(&pongGame);
  registerGame(&tetrisGame);
  registerGame(&flappyGame);
  registerGame(&asteroidsGame);
  registerGame(&breakoutGame);
  registerGame(&conwayGame);

  // ── Register Visual Demos (category 1) ──
  registerGame(&kaleidoscopeDemo);
  registerGame(&matrixDemo);
  registerGame(&mazeDemo);
  registerGame(&particlesDemo);
  registerGame(&fractalsDemo);
  registerGame(&slimeDemo);

  Serial.printf("[OK] %d apps registered\n", gameCount);

  // ── Splash ──
  showSplash();

  // ── Boot into Menu ──
  Buzzer::play(Buzzer::Sound::Confirm);
  initMenu();

  Serial.println("[OK] Menu active — ready to play!");
}

// ═══════════════════════════════════════════════════════════
//  LOOP — runs forever
// ═══════════════════════════════════════════════════════════
void loop() {
  handleMenuInput();
}