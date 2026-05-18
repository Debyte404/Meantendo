#include "../core/Game.hpp"
#include "../core/Input.hpp"
#include <Adafruit_ST7735.h>
#include <math.h>

extern Adafruit_ST7735 tft;

// ═══════════════════════════════════════════════════════════
//  ASTEROIDS — Vector-style space shooter
//  160×128 display
// ═══════════════════════════════════════════════════════════

#define AST_MAX_ASTEROIDS 12
#define AST_MAX_BULLETS   6
#define AST_TURN_SPEED    0.12f
#define AST_THRUST        0.15f
#define AST_FRICTION      0.98f
#define AST_BULLET_SPD    4.0f
#define AST_BULLET_LIFE   40

static const uint16_t AST_SHIP_COL  = ST77XX_CYAN;
static const uint16_t AST_ROCK_COL  = tft.color565(180, 180, 180);
static const uint16_t AST_BULL_COL  = ST77XX_YELLOW;

struct Ship {
  float x, y, angle, vx, vy;
  bool alive;
};

struct Bullet {
  float x, y, vx, vy;
  int life;
  bool active;
};

struct Asteroid {
  float x, y, vx, vy, radius;
  bool active;
};

static Ship ship;
static Bullet bullets[AST_MAX_BULLETS];
static Asteroid asteroids[AST_MAX_ASTEROIDS];
static int astScore;
static bool astGameOver;
static int astLevel;
static bool astAutoFire;

static float wrapX(float x) { float w = tft.width();  while(x<0) x+=w; while(x>=w) x-=w; return x; }
static float wrapY(float y) { float h = tft.height(); while(y<0) y+=h; while(y>=h) y-=h; return y; }

static void drawShip(uint16_t color) {
  float ca = cos(ship.angle), sa = sin(ship.angle);
  // Nose, left wing, right wing
  int x0 = ship.x + ca * 7,  y0 = ship.y + sa * 7;
  int x1 = ship.x + cos(ship.angle + 2.4f) * 6, y1 = ship.y + sin(ship.angle + 2.4f) * 6;
  int x2 = ship.x + cos(ship.angle - 2.4f) * 6, y2 = ship.y + sin(ship.angle - 2.4f) * 6;
  tft.drawTriangle(x0, y0, x1, y1, x2, y2, color);
}

static void drawAsteroid(Asteroid &a, uint16_t color) {
  tft.drawCircle((int)a.x, (int)a.y, (int)a.radius, color);
  // Inner detail line for flavor
  if (a.radius > 6) {
    tft.drawCircle((int)a.x + 1, (int)a.y - 1, (int)(a.radius * 0.5f), color);
  }
}

static void spawnAsteroids(int count, float minR, float maxR) {
  for (int i = 0; i < AST_MAX_ASTEROIDS && count > 0; i++) {
    if (!asteroids[i].active) {
      asteroids[i].active = true;
      asteroids[i].x = random(0, tft.width());
      asteroids[i].y = random(0, tft.height());
      float spd = random(50, 120) / 100.0f;
      float ang = random(0, 628) / 100.0f;
      asteroids[i].vx = cos(ang) * spd;
      asteroids[i].vy = sin(ang) * spd;
      asteroids[i].radius = random((int)minR, (int)maxR + 1);
      // Don't spawn on top of ship
      float dx = asteroids[i].x - ship.x;
      float dy = asteroids[i].y - ship.y;
      if (sqrt(dx*dx + dy*dy) < 30) {
        asteroids[i].x = wrapX(asteroids[i].x + 50);
      }
      count--;
    }
  }
}

static void fireBullet() {
  for (int i = 0; i < AST_MAX_BULLETS; i++) {
    if (!bullets[i].active) {
      bullets[i].active = true;
      bullets[i].x = ship.x + cos(ship.angle) * 8;
      bullets[i].y = ship.y + sin(ship.angle) * 8;
      bullets[i].vx = cos(ship.angle) * AST_BULLET_SPD;
      bullets[i].vy = sin(ship.angle) * AST_BULLET_SPD;
      bullets[i].life = AST_BULLET_LIFE;
      return;
    }
  }
}

static bool anyAsteroidsLeft() {
  for (int i = 0; i < AST_MAX_ASTEROIDS; i++)
    if (asteroids[i].active) return true;
  return false;
}

void startAsteroids() {
  randomSeed(esp_random());
  ship = { (float)(tft.width()/2), (float)(tft.height()/2), -1.57f, 0, 0, true };
  memset(bullets, 0, sizeof(bullets));
  memset(asteroids, 0, sizeof(asteroids));
  astScore = 0;
  astGameOver = false;
  astLevel = 1;
  astAutoFire = false;

  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(10, 20);
  tft.print("ASTEROIDS");

  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(5, 50);  tft.print("B: Thrust Fwd");
  tft.setCursor(5, 65);  tft.print("Y: Thrust Back");
  tft.setCursor(5, 80);  tft.print("A: Auto-Fire Tgl");
  tft.setCursor(5, 95);  tft.print("JOY: Steer");
  
  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(15, 115); tft.print("Press X to start");

  while (!xPressed()) delay(10);
  while (xPressed()) delay(10); // Wait for release

  tft.fillScreen(ST77XX_BLACK);
  spawnAsteroids(3 + astLevel, 8, 14);
}

void loopAsteroids() {
  if (astGameOver) {
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_RED);
    tft.setCursor(15, 35);
    tft.print("GAME OVER");
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(35, 60);
    tft.printf("Score: %d", astScore);
    tft.setCursor(30, 78);
    tft.setTextColor(ST77XX_WHITE);
    tft.print("Press BACK");
    delay(400);
    while (!backPressed()) delay(50);
    return;
  }

  // === Erase everything ===
  drawShip(ST77XX_BLACK);
  for (int i = 0; i < AST_MAX_BULLETS; i++)
    if (bullets[i].active)
      tft.drawPixel((int)bullets[i].x, (int)bullets[i].y, ST77XX_BLACK);
  for (int i = 0; i < AST_MAX_ASTEROIDS; i++)
    if (asteroids[i].active)
      drawAsteroid(asteroids[i], ST77XX_BLACK);

  // === Input ===
  int jx = readJoystickAnalogX();

  if (jx > JOY_CENTER) ship.angle -= AST_TURN_SPEED; // Left
  if (jx < JOY_CENTER) ship.angle += AST_TURN_SPEED; // Right

  if (bPressed()) { // Forward Thrust
    ship.vx += cos(ship.angle) * AST_THRUST;
    ship.vy += sin(ship.angle) * AST_THRUST;
  }
  if (yPressed()) { // Backward Thrust
    ship.vx -= cos(ship.angle) * AST_THRUST;
    ship.vy -= sin(ship.angle) * AST_THRUST;
  }

  if (debouncedPress(BTN_A, 150)) {
    astAutoFire = !astAutoFire;
  }
  
  static unsigned long lastFire = 0;
  if (astAutoFire && millis() - lastFire > 180) {
    fireBullet();
    lastFire = millis();
  }

  // === Update ship ===
  ship.vx *= AST_FRICTION;
  ship.vy *= AST_FRICTION;
  ship.x = wrapX(ship.x + ship.vx);
  ship.y = wrapY(ship.y + ship.vy);

  // === Update bullets ===
  for (int i = 0; i < AST_MAX_BULLETS; i++) {
    if (!bullets[i].active) continue;
    bullets[i].x = wrapX(bullets[i].x + bullets[i].vx);
    bullets[i].y = wrapY(bullets[i].y + bullets[i].vy);
    bullets[i].life--;
    if (bullets[i].life <= 0) bullets[i].active = false;
  }

  // === Update asteroids ===
  for (int i = 0; i < AST_MAX_ASTEROIDS; i++) {
    if (!asteroids[i].active) continue;
    asteroids[i].x = wrapX(asteroids[i].x + asteroids[i].vx);
    asteroids[i].y = wrapY(asteroids[i].y + asteroids[i].vy);
  }

  // === Bullet-Asteroid collision ===
  for (int b = 0; b < AST_MAX_BULLETS; b++) {
    if (!bullets[b].active) continue;
    for (int a = 0; a < AST_MAX_ASTEROIDS; a++) {
      if (!asteroids[a].active) continue;
      float dx = bullets[b].x - asteroids[a].x;
      float dy = bullets[b].y - asteroids[a].y;
      if (dx*dx + dy*dy < asteroids[a].radius * asteroids[a].radius) {
        bullets[b].active = false;
        // Split large asteroids
        if (asteroids[a].radius > 6) {
          float nr = asteroids[a].radius * 0.55f;
          for (int k = 0; k < AST_MAX_ASTEROIDS && k < 2; k++) {
            for (int j = 0; j < AST_MAX_ASTEROIDS; j++) {
              if (!asteroids[j].active) {
                asteroids[j].active = true;
                asteroids[j].x = asteroids[a].x;
                asteroids[j].y = asteroids[a].y;
                float ang = random(0, 628) / 100.0f;
                float spd = random(80, 150) / 100.0f;
                asteroids[j].vx = cos(ang) * spd;
                asteroids[j].vy = sin(ang) * spd;
                asteroids[j].radius = nr;
                break;
              }
            }
          }
        }
        asteroids[a].active = false;
        astScore += 10;
        break;
      }
    }
  }

  // === Ship-Asteroid collision ===
  for (int a = 0; a < AST_MAX_ASTEROIDS; a++) {
    if (!asteroids[a].active) continue;
    float dx = ship.x - asteroids[a].x;
    float dy = ship.y - asteroids[a].y;
    if (dx*dx + dy*dy < (asteroids[a].radius + 4) * (asteroids[a].radius + 4)) {
      astGameOver = true;
      return;
    }
  }

  // === Level up ===
  if (!anyAsteroidsLeft()) {
    astLevel++;
    spawnAsteroids(3 + astLevel, 8, 14);
  }

  // === Draw everything ===
  drawShip(AST_SHIP_COL);
  for (int i = 0; i < AST_MAX_BULLETS; i++)
    if (bullets[i].active)
      tft.drawPixel((int)bullets[i].x, (int)bullets[i].y, AST_BULL_COL);
  for (int i = 0; i < AST_MAX_ASTEROIDS; i++)
    if (asteroids[i].active)
      drawAsteroid(asteroids[i], AST_ROCK_COL);

  // Score HUD
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(2, 2);
  tft.printf("%-5d", astScore);

  delay(25);
}

GameDef asteroidsGame = { "Asteroids", startAsteroids, loopAsteroids, 0 };
