#include "../core/Game.hpp"
#include "../core/Input.hpp"
#include <Adafruit_ST7735.h>

extern Adafruit_ST7735 tft;

// ═══════════════════════════════════════════════════════════
//  MATRIX RAIN — Digital rain screensaver
//  Falling green characters like "The Matrix"
// ═══════════════════════════════════════════════════════════

#define MR_COLS     26  // 160 / 6 = ~26 columns
#define MR_CHARH    7   // character height in pixels
#define MR_MAX_ROWS 18  // 128 / 7 = ~18 rows

struct RainColumn {
  int headY;        // current head position (in row units)
  int length;       // trail length
  int speed;        // frames between advances
  int timer;        // countdown to next advance
};

static RainColumn columns[MR_COLS];

static char randomChar() {
  // Mix of digits, latin, and pseudo-katakana range
  int r = random(0, 3);
  if (r == 0) return '0' + random(0, 10);
  if (r == 1) return 'A' + random(0, 26);
  return random(33, 127); // extended ASCII printable
}

static uint16_t greenShade(int brightness) {
  // brightness: 0 (dark) to 255 (bright)
  int g = constrain(brightness, 0, 255);
  int r = g / 8;  // slight warm tint
  return tft.color565(r, g, 0);
}

static void resetColumn(int i) {
  columns[i].headY = -random(0, MR_MAX_ROWS);
  columns[i].length = random(4, MR_MAX_ROWS - 2);
  columns[i].speed = random(1, 4);
  columns[i].timer = columns[i].speed;
}

void startMatrixRain() {
  tft.fillScreen(ST77XX_BLACK);
  for (int i = 0; i < MR_COLS; i++) {
    resetColumn(i);
  }
}

void loopMatrixRain() {
  for (int i = 0; i < MR_COLS; i++) {
    columns[i].timer--;
    if (columns[i].timer > 0) continue;
    columns[i].timer = columns[i].speed;

    int x = i * 6 + 1;

    // Draw bright head character
    if (columns[i].headY >= 0 && columns[i].headY < MR_MAX_ROWS) {
      int y = columns[i].headY * MR_CHARH;
      tft.setTextColor(ST77XX_WHITE); // brightest = white
      tft.setTextSize(1);
      tft.setCursor(x, y);
      char buf[2] = { randomChar(), 0 };
      tft.print(buf);
    }

    // Fade the character that was previously the head (now trail)
    int trailY = columns[i].headY - 1;
    if (trailY >= 0 && trailY < MR_MAX_ROWS) {
      int y = trailY * MR_CHARH;
      tft.setTextColor(greenShade(200));
      tft.setCursor(x, y);
      char buf[2] = { randomChar(), 0 };
      tft.print(buf);
    }

    // Dim older trail characters
    for (int t = 2; t < columns[i].length; t++) {
      int ty = columns[i].headY - t;
      if (ty >= 0 && ty < MR_MAX_ROWS) {
        int brightness = 200 - (t * 200 / columns[i].length);
        if (brightness < 20) brightness = 20;
        int y = ty * MR_CHARH;
        tft.setTextColor(greenShade(brightness));
        tft.setCursor(x, y);
        char buf[2] = { randomChar(), 0 };
        tft.print(buf);
      }
    }

    // Erase tail
    int tailY = columns[i].headY - columns[i].length;
    if (tailY >= 0 && tailY < MR_MAX_ROWS) {
      int y = tailY * MR_CHARH;
      tft.fillRect(x, y, 6, MR_CHARH, ST77XX_BLACK);
    }

    columns[i].headY++;

    // Reset when fully scrolled off
    if (columns[i].headY - columns[i].length > MR_MAX_ROWS) {
      resetColumn(i);
    }
  }

  delay(30);
}

GameDef matrixDemo = { "Matrix", startMatrixRain, loopMatrixRain, 1 };
