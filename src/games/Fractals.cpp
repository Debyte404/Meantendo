#include "../core/Game.hpp"
#include "../core/Input.hpp"
#include <Adafruit_ST7735.h>
#include <math.h>

extern Adafruit_ST7735 tft;

// ═══════════════════════════════════════════════════════════
//  FRACTALS — Mandelbrot set auto-zoom explorer
//  Renders pixel-by-pixel, then zooms into interesting spots
// ═══════════════════════════════════════════════════════════

#define FR_MAX_ITER 40
#define FR_W tft.width()
#define FR_H tft.height()

// Zoom targets — interesting regions of the Mandelbrot set
struct ZoomTarget {
  double cx, cy;     // center coordinates
  double startZoom;
  double endZoom;
  const char* name;
};

static const ZoomTarget targets[] = {
  { -0.7436447860,  0.1318252536, 0.5, 0.0001,  "Seahorse"  },
  { -0.1011,        0.9563,       0.5, 0.001,    "Spiral"    },
  { -1.2500,        0.0000,       0.5, 0.0005,   "Needle"    },
  {  0.2825,        0.0100,       0.4, 0.0008,   "Elephant"  },
  { -0.7463,        0.1102,       0.5, 0.0003,   "Valley"    },
};
static const int NUM_TARGETS = 5;

static int currentTarget;
static double zoomLevel;
static double zoomSpeed;
static int renderY;  // current row being rendered
static bool rendering;

// Color palette — map iteration count to color
static uint16_t iterToColor(int iter) {
  if (iter >= FR_MAX_ITER) return ST77XX_BLACK;

  // Smooth color cycling through a vibrant palette
  float t = (float)iter / FR_MAX_ITER;

  uint8_t r, g, b;
  if (t < 0.16f) {
    r = 0; g = 0; b = (uint8_t)(t / 0.16f * 180);
  } else if (t < 0.42f) {
    float s = (t - 0.16f) / 0.26f;
    r = 0; g = (uint8_t)(s * 255); b = (uint8_t)(180 - s * 80);
  } else if (t < 0.6425f) {
    float s = (t - 0.42f) / 0.2225f;
    r = (uint8_t)(s * 255); g = 255; b = (uint8_t)(100 - s * 100);
  } else if (t < 0.8575f) {
    float s = (t - 0.6425f) / 0.215f;
    r = 255; g = (uint8_t)(255 - s * 200); b = 0;
  } else {
    float s = (t - 0.8575f) / 0.1425f;
    r = (uint8_t)(255 - s * 200); g = (uint8_t)(55 - s * 55); b = 0;
  }

  return tft.color565(r, g, b);
}

static void renderRow(int row) {
  const ZoomTarget &tgt = targets[currentTarget];
  double scale = zoomLevel;

  for (int px = 0; px < FR_W; px++) {
    double x0 = tgt.cx + (px - FR_W / 2.0) * scale / FR_W;
    double y0 = tgt.cy + (row - FR_H / 2.0) * scale / FR_H;

    double x = 0, y = 0;
    int iter = 0;

    while (x * x + y * y <= 4.0 && iter < FR_MAX_ITER) {
      double xtemp = x * x - y * y + x0;
      y = 2 * x * y + y0;
      x = xtemp;
      iter++;
    }

    tft.drawPixel(px, row, iterToColor(iter));
  }
}

void startFractals() {
  randomSeed(esp_random());
  currentTarget = random(0, NUM_TARGETS);
  zoomLevel = targets[currentTarget].startZoom;
  zoomSpeed = 0.85;
  renderY = 0;
  rendering = true;

  tft.fillScreen(ST77XX_BLACK);

  // Show target name briefly
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(5, 5);
  tft.print("Fractal: ");
  tft.print(targets[currentTarget].name);
}

void loopFractals() {
  if (rendering) {
    // Render a few rows per frame to keep it responsive
    for (int i = 0; i < 4 && renderY < FR_H; i++) {
      renderRow(renderY);
      renderY++;
    }

    if (renderY >= FR_H) {
      // Frame complete — zoom in and re-render
      rendering = false;
      delay(800);
    }
  } else {
    // Zoom step
    zoomLevel *= zoomSpeed;

    // If we've reached the target zoom, switch to next region
    if (zoomLevel <= targets[currentTarget].endZoom) {
      currentTarget = (currentTarget + 1) % NUM_TARGETS;
      zoomLevel = targets[currentTarget].startZoom;

      tft.fillScreen(ST77XX_BLACK);
      tft.setTextSize(1);
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(5, 5);
      tft.print("Fractal: ");
      tft.print(targets[currentTarget].name);
      delay(300);
    }

    renderY = 0;
    rendering = true;
  }
}

GameDef fractalsDemo = { "Fractal", startFractals, loopFractals, 1 };
