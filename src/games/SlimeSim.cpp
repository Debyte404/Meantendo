#include "../core/Game.hpp"
#include "../core/Input.hpp"
#include <Adafruit_ST7735.h>
#include <math.h>

extern Adafruit_ST7735 tft;

// ═══════════════════════════════════════════════════════════
//  SLIME MOLD SIMULATION — Physarum Polycephalum
// ═══════════════════════════════════════════════════════════

#define SL_W 80
#define SL_H 64
#define AGENT_COUNT 600

// Agent Params
#define SENSE_DIST 3.0f
#define SENSE_ANGLE 0.4f
#define TURN_SPEED 0.25f
#define MOVE_SPEED 1.0f
#define DEPOSIT_AMT 35
#define DECAY_AMT 3

struct Agent {
  float x, y;
  float angle;
};

// Double-buffered grid for blur
static uint8_t *trailMap = nullptr;
static uint8_t *blurMap = nullptr;
static Agent *agents = nullptr;
static float slimeHue = 0;

// Get density with bounds check
static uint8_t sense(float x, float y, float angle) {
  int sx = (int)(x + cos(angle) * SENSE_DIST);
  int sy = (int)(y + sin(angle) * SENSE_DIST);
  if (sx < 0 || sx >= SL_W || sy < 0 || sy >= SL_H) return 0;
  return trailMap[sy * SL_W + sx];
}

void startSlime() {
  if (!trailMap) trailMap = (uint8_t*)malloc(SL_W * SL_H);
  if (!blurMap) blurMap = (uint8_t*)malloc(SL_W * SL_H);
  if (!agents) agents = (Agent*)malloc(AGENT_COUNT * sizeof(Agent));

  memset(trailMap, 0, SL_W * SL_H);
  slimeHue = random(0, 360);

  // Spawn agents in circle
  for (int i = 0; i < AGENT_COUNT; i++) {
    float r = random(0, 1500) / 100.0f;
    float theta = random(0, 628) / 100.0f;
    agents[i].x = SL_W / 2 + cos(theta) * r;
    agents[i].y = SL_H / 2 + sin(theta) * r;
    // Walk towards center
    agents[i].angle = theta + PI;
  }

  tft.fillScreen(ST77XX_BLACK);
}

void loopSlime() {
  if (!trailMap || !blurMap || !agents) {
    // Memory alloc failed
    delay(100);
    return;
  }

  // 1. Move Agents & Deposit
  for (int i = 0; i < AGENT_COUNT; i++) {
    Agent &a = agents[i];

    float weightF = sense(a.x, a.y, a.angle);
    float weightL = sense(a.x, a.y, a.angle - SENSE_ANGLE);
    float weightR = sense(a.x, a.y, a.angle + SENSE_ANGLE);

    // Steer
    float randV = random(0, 100) / 10000.0f; // tiny random wander
    if (weightF > weightL && weightF > weightR) {
      a.angle += 0;
    } else if (weightF < weightL && weightF < weightR) {
      if (random(0,2) == 0) a.angle -= TURN_SPEED;
      else a.angle += TURN_SPEED;
    } else if (weightL > weightR) {
      a.angle -= TURN_SPEED;
    } else if (weightR > weightL) {
      a.angle += TURN_SPEED;
    }
    a.angle += randV;

    // Move
    float nx = a.x + cos(a.angle) * MOVE_SPEED;
    float ny = a.y + sin(a.angle) * MOVE_SPEED;

    // Wrap around bounds
    if (nx < 0) nx += SL_W; if (nx >= SL_W) nx -= SL_W;
    if (ny < 0) ny += SL_H; if (ny >= SL_H) ny -= SL_H;
    a.x = nx; a.y = ny;

    // Deposit
    int idx = (int)a.y * SL_W + (int)a.x;
    int v = trailMap[idx] + DEPOSIT_AMT;
    trailMap[idx] = (v > 255) ? 255 : v;
  }

  // Hue cycling over time
  slimeHue += 0.2f;
  if(slimeHue >= 360.0f) slimeHue -= 360.0f;
  
  // Base neon hue to RGB conversion
  float s = 1.0f;
  float c = s; 
  float x = c * (1.0f - fabs(fmod(slimeHue / 60.0f, 2.0f) - 1.0f));
  float br = 0, bg = 0, bb = 0;
  if      (slimeHue < 60)  { br = c; bg = x; }
  else if (slimeHue < 120) { br = x; bg = c; }
  else if (slimeHue < 180) { bg = c; bb = x; }
  else if (slimeHue < 240) { bg = x; bb = c; }
  else if (slimeHue < 300) { br = x; bb = c; }
  else                     { br = c; bb = x; }

  // 2. Diffuse, Decay, and Draw
  for (int y = 0; y < SL_H; y++) {
    for (int x = 0; x < SL_W; x++) {
      // 3x3 Average (diffusion blur)
      int sum = 0;
      for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
          int nx = x + dx; int ny = y + dy;
          if (nx < 0) nx += SL_W; else if (nx >= SL_W) nx -= SL_W;
          if (ny < 0) ny += SL_H; else if (ny >= SL_H) ny -= SL_H;
          sum += trailMap[ny * SL_W + nx];
        }
      }
      
      int avg = sum / 9;
      // Decay
      int nextVal = avg - DECAY_AMT;
      if (nextVal < 0) nextVal = 0;
      blurMap[y * SL_W + x] = nextVal;
      
      // Draw to screen (only if changed significantly to save SPI time, or if bright)
      if (nextVal > 0) {
        // Apply neon hue to brightness
        float v = nextVal / 255.0f;
        uint16_t color = tft.color565((uint8_t)(br*v*255), (uint8_t)(bg*v*255), (uint8_t)(bb*v*255));
        tft.fillRect(x * 2, y * 2, 2, 2, color);
      } else if (trailMap[y * SL_W + x] > 0) {
        // Fade out
        tft.fillRect(x * 2, y * 2, 2, 2, ST77XX_BLACK);
      }
    }
  }

  // Swap buffers
  uint8_t *tmp = trailMap;
  trailMap = blurMap;
  blurMap = tmp;
}

GameDef slimeDemo = { "SlimeMold", startSlime, loopSlime, 1 };
