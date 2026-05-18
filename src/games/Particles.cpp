#include "../core/Game.hpp"
#include "../core/Input.hpp"
#include <Adafruit_ST7735.h>
#include <math.h>

extern Adafruit_ST7735 tft;

// ═══════════════════════════════════════════════════════════
//  SPARKS (PARTICLES) — Advanced Physics with Trails & Drag
// ═══════════════════════════════════════════════════════════

#define PT_MAX 80

struct Particle {
  float x, y, px, py; // current and previous position for trails
  float vx, vy;
  uint8_t life;    // 0 = dead
  uint8_t maxLife;
  uint8_t hue;
};

static Particle particles[PT_MAX];
static int ptMode;
static unsigned long ptLastModeSw;
static float ptTime;
static int fadeCounter = 0;

static uint16_t getSparkColor(uint8_t hue, uint8_t life, uint8_t maxLife) {
  // Map hue across fully saturated neon palette
  float h = hue * 360.0f / 255.0f;
  float s = 1.0f;
  float v = (float)life / maxLife; // Fade over time
  if (v < 0) v = 0;

  // HSV to RGB
  float c = v * s;
  float x = c * (1.0f - fabs(fmod(h / 60.0f, 2.0f) - 1.0f));
  float m = v - c;
  float r = 0, g = 0, b = 0;
  
  if      (h < 60)  { r = c; g = x; b = 0; }
  else if (h < 120) { r = x; g = c; b = 0; }
  else if (h < 180) { r = 0; g = c; b = x; }
  else if (h < 240) { r = 0; g = x; b = c; }
  else if (h < 300) { r = x; g = 0; b = c; }
  else              { r = c; g = 0; b = x; }
  
  // Towards end of life, sparks turn white-hot or slightly dimmer
  if (v > 0.8f) { r += (v-0.8f)*5.0f; g += (v-0.8f)*5.0f; b += (v-0.8f)*5.0f; }
  
  if (r > 1.0f) r = 1.0f; if (g > 1.0f) g = 1.0f; if (b > 1.0f) b = 1.0f;
  
  return tft.color565((uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255));
}

static void spawnBurst(float cx, float cy, int count, int type) {
  uint8_t baseHue = random(0, 255);
  for (int i = 0; i < PT_MAX && count > 0; i++) {
    if (particles[i].life > 0) continue;
    float angle, speed;
    if (type == 0) { // Circle Burst
      angle = random(0, 628) / 100.0f;
      speed = random(50, 400) / 100.0f;
      particles[i].hue = baseHue + random(-20, 20);
    } else if (type == 1) { // Ring Burst
      angle = (float)count * 0.5f; // Evenly spaced
      speed = 3.5f + random(-20, 20)/100.0f;
      particles[i].hue = baseHue + count * 5;
    } else { // Fountain
      angle = random(-140, -40) * PI / 180.0f;
      speed = random(200, 450) / 100.0f;
      particles[i].hue = baseHue + random(-50, 50);
    }
    
    particles[i].x = cx; particles[i].px = cx;
    particles[i].y = cy; particles[i].py = cy;
    particles[i].vx = cos(angle) * speed;
    particles[i].vy = sin(angle) * speed;
    particles[i].life = random(30, 70);
    particles[i].maxLife = particles[i].life;
    count--;
  }
}

void startParticles() {
  memset(particles, 0, sizeof(particles));
  ptMode = 0;
  ptLastModeSw = millis();
  ptTime = 0;
  fadeCounter = 0;
  tft.fillScreen(ST77XX_BLACK);
}

void loopParticles() {
  // Intensified Fading Trails Effect
  fadeCounter++;
  int step = (fadeCounter % 3) + 1; // 1, 2, or 3
  for (int y = 0; y < tft.height(); y += 2) {
    for (int x = (y + step) % 4; x < tft.width(); x += 4) {
      tft.drawPixel(x, y, ST77XX_BLACK);
    }
  }

  // Scene Director
  if (millis() - ptLastModeSw > 2500) {
    ptMode = random(0, 3);
    ptLastModeSw = millis();
    if (ptMode == 0 || ptMode == 1) {
      spawnBurst(random(30, 130), random(30, 90), 40, ptMode);
    }
  }
  
  if (ptMode == 2) { // Continuous fountain
    spawnBurst(80, 128, 3, 2);
  }

  // Update & Draw
  for (int i = 0; i < PT_MAX; i++) {
    if (particles[i].life == 0) continue;

    // Erase old tail end by re-dimming that pixel? Better to let the fade loop do it,
    // but the trails look much better using drawLine!

    particles[i].px = particles[i].x;
    particles[i].py = particles[i].y;
    
    // Physics
    particles[i].x += particles[i].vx;
    particles[i].y += particles[i].vy;
    particles[i].vy += 0.04f; // softer gravity
    
    // Air drag creates cluster-explosions!
    particles[i].vx *= 0.95f;
    particles[i].vy *= 0.96f; 
    
    particles[i].life--;

    if (particles[i].x < 0 || particles[i].x >= tft.width() || particles[i].y >= tft.height()) {
      particles[i].life = 0;
      continue;
    }

    uint16_t color = getSparkColor(particles[i].hue, particles[i].life, particles[i].maxLife);
    tft.drawLine((int)particles[i].px, (int)particles[i].py, (int)particles[i].x, (int)particles[i].y, color);
  }

  delay(20);
}

GameDef particlesDemo = { "Sparks", startParticles, loopParticles, 1 };
