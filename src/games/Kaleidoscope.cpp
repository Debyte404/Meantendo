#include "../core/Game.hpp"
#include "../core/Input.hpp"
#include <Adafruit_ST7735.h>
#include <math.h>

extern Adafruit_ST7735 tft;

// ═══════════════════════════════════════════════════════════
//  KALEIDOSCOPE — Morphing 8-fold solid geometry
// ═══════════════════════════════════════════════════════════

static float kTime = 0;
static int fadePhase = 0;

static uint16_t getNeonColor(float phase) {
  float h = fmod(phase, 360.0f);
  float s = 1.0f;
  float v = 1.0f;
  
  float c = v * s;
  float x = c * (1.0f - fabs(fmod(h / 60.0f, 2.0f) - 1.0f));
  float m = v - c;
  float r = 0, g = 0, b = 0;
  
  if      (h < 60)  { r = c; g = x; }
  else if (h < 120) { r = x; g = c; }
  else if (h < 180) { g = c; b = x; }
  else if (h < 240) { g = x; b = c; }
  else if (h < 300) { r = x; b = c; }
  else              { r = c; b = x; }
  
  return tft.color565((r+m)*255, (g+m)*255, (b+m)*255);
}

void startKaleidoscope() {
  kTime = 0;
  tft.fillScreen(ST77XX_BLACK);
}

void loopKaleidoscope() {
  int cx = tft.width() / 2;
  int cy = tft.height() / 2;

  // We draw a few layers of geometry per frame to create depth
  for(int layer = 0; layer < 2; layer++) {
    float layerOffset = layer * 40.0f; // phase offset
    float rad1 = 10.0f + 50.0f * fabs(sin(kTime * 0.4f + layerOffset*0.01f));
    float rad2 = 10.0f + 60.0f * fabs(cos(kTime * 0.3f - layerOffset*0.01f));
    
    // Angle oscillates slightly to "twist" the shapes
    float baseAngle = kTime * 0.15f * (layer == 0 ? 1 : -1);
    float spread = (PI / 8.0f) * fabs(cos(kTime * 0.5f)); 

    uint16_t color = getNeonColor(kTime * 60.0f + layerOffset);
    uint16_t edgeColor = getNeonColor(kTime * 60.0f + layerOffset + 30.0f);

    // 8-Fold symmetry
    for (int m = 0; m < 8; m++) {
      float mAngle = baseAngle + m * (PI / 4.0f);
      
      int px1 = cx + cos(mAngle - spread) * rad1;
      int py1 = cy + sin(mAngle - spread) * rad1;
      int px2 = cx + cos(mAngle + spread) * rad2;
      int py2 = cy + sin(mAngle + spread) * rad2;
      
      tft.fillTriangle(cx, cy, px1, py1, px2, py2, color);
      tft.drawTriangle(cx, cy, px1, py1, px2, py2, edgeColor);
    }
  }

  kTime += 0.05f;

  // Fade effect (organic dimming via pseudo-random pixel overdraw)
  fadePhase++;
  for(int p = 0; p < 800; p++) {
    int x = random(0, tft.width());
    int y = random(0, tft.height());
    tft.drawPixel(x, y, ST77XX_BLACK);
  }

  delay(25);
}

GameDef kaleidoscopeDemo = { "Kaleido", startKaleidoscope, loopKaleidoscope, 1 };
