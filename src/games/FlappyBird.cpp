#include "../core/Game.hpp"
#include "../core/Input.hpp"
#include <Adafruit_ST7735.h>

extern Adafruit_ST7735 tft;

// ═══════════════════════════════════════════════════════════
//  FLAPPY BIRD — Tap to flap, dodge pipes
//  160×128 display
// ═══════════════════════════════════════════════════════════

#define FB_GRAVITY    0.45f
#define FB_FLAP      -3.2f
#define FB_BIRD_X    25
#define FB_BIRD_W    8
#define FB_BIRD_H    6
#define FB_PIPE_W    12
#define FB_GAP       36
#define FB_PIPE_SPD  2
#define FB_MAX_PIPES 3

#define FB_SKY    tft.color565(40, 40, 80)
#define FB_GROUND tft.color565(80, 60, 30)
#define FB_PIPE   tft.color565(20, 140, 40)
#define FB_BIRD   tft.color565(255, 220, 50)
#define FB_BEAK   tft.color565(255, 120, 30)
#define FB_EYE    ST77XX_WHITE

static float birdY, birdVel;
static int pipeX[FB_MAX_PIPES];
static int pipeGapY[FB_MAX_PIPES];
static int fbScore;
static bool fbGameOver;
static bool fbStarted;
static int groundY;

static void resetPipe(int i, int startX) {
  pipeX[i] = startX;
  pipeGapY[i] = random(20, groundY - FB_GAP - 10);
}

static void drawBird(int y, uint16_t bodyColor) {
  // Body
  tft.fillRect(FB_BIRD_X, y, FB_BIRD_W, FB_BIRD_H, bodyColor);
  if (bodyColor != FB_SKY) {
    // Wing
    tft.fillRect(FB_BIRD_X + 1, y + 2, 4, 3, tft.color565(255, 180, 0));
    // Eye
    tft.drawPixel(FB_BIRD_X + FB_BIRD_W - 2, y + 1, FB_EYE);
    // Beak
    tft.fillRect(FB_BIRD_X + FB_BIRD_W, y + 2, 3, 2, FB_BEAK);
  } else {
    // Erase beak area too
    tft.fillRect(FB_BIRD_X + FB_BIRD_W, y + 1, 3, 3, FB_SKY);
  }
}

static void drawPipe(int x, int gapY, uint16_t color) {
  if (x < -FB_PIPE_W - 2 || x > tft.width() + 2) return;
  // Top pipe
  int topH = gapY;
  if (topH > 0)
    tft.fillRect(x, 0, FB_PIPE_W, topH, color);
  // Bottom pipe
  int botY = gapY + FB_GAP;
  int botH = groundY - botY;
  if (botH > 0)
    tft.fillRect(x, botY, FB_PIPE_W, botH, color);
  // Pipe lips
  if (color != FB_SKY) {
    tft.drawRect(x - 1, gapY - 3, FB_PIPE_W + 2, 4, tft.color565(10, 100, 20));
    tft.drawRect(x - 1, botY, FB_PIPE_W + 2, 4, tft.color565(10, 100, 20));
  } else {
    tft.drawRect(x - 1, gapY - 3, FB_PIPE_W + 2, 4, FB_SKY);
    tft.drawRect(x - 1, botY, FB_PIPE_W + 2, 4, FB_SKY);
  }
}

static void drawGround() {
  tft.fillRect(0, groundY, tft.width(), tft.height() - groundY, FB_GROUND);
  tft.drawFastHLine(0, groundY, tft.width(), tft.color565(60, 40, 20));
}

static void drawScore() {
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE, FB_SKY);
  tft.setCursor(tft.width() / 2 - 6, 4);
  tft.printf("%d", fbScore);
}

// === Start ===
void startFlappy() {
  randomSeed(esp_random());
  groundY = tft.height() - 12;
  birdY = groundY / 2.0f;
  birdVel = 0;
  fbScore = 0;
  fbGameOver = false;
  fbStarted = false;

  tft.fillScreen(FB_SKY);
  drawGround();

  for (int i = 0; i < FB_MAX_PIPES; i++) {
    resetPipe(i, tft.width() + i * 65);
  }

  drawBird((int)birdY, FB_BIRD);

  // Title
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(20, 25);
  tft.print("FLAPPY");
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(35, 50);
  tft.print("Press A to fly");
}

// === Loop ===
void loopFlappy() {
  if (fbGameOver) {
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_RED);
    tft.setCursor(20, 35);
    tft.print("GAME OVER");
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(35, 60);
    tft.printf("Score: %d", fbScore);
    tft.setCursor(30, 80);
    tft.setTextColor(ST77XX_WHITE);
    tft.print("Press BACK");
    delay(400);
    while (!backPressed()) delay(50);
    return;
  }

  // Flap input
  bool flap = debouncedPress(BTN_A, 120);

  if (flap && !fbStarted) {
    fbStarted = true;
    tft.fillScreen(FB_SKY);
    drawGround();
  }

  if (!fbStarted) return;

  if (flap) birdVel = FB_FLAP;

  // Erase old bird
  drawBird((int)birdY, FB_SKY);

  // Physics
  birdVel += FB_GRAVITY;
  birdY += birdVel;
  if (birdY < 0) birdY = 0;

  // Ground collision
  if (birdY + FB_BIRD_H >= groundY) {
    birdY = groundY - FB_BIRD_H;
    fbGameOver = true;
  }

  // Move and draw pipes
  for (int i = 0; i < FB_MAX_PIPES; i++) {
    drawPipe(pipeX[i], pipeGapY[i], FB_SKY);
    pipeX[i] -= FB_PIPE_SPD;

    // Scoring — bird just passed the pipe's right edge
    if (pipeX[i] + FB_PIPE_W == FB_BIRD_X) {
      fbScore++;
    }

    // Reset pipe when it scrolls off screen
    if (pipeX[i] < -FB_PIPE_W) {
      resetPipe(i, tft.width() + random(20, 40));
    }

    drawPipe(pipeX[i], pipeGapY[i], FB_PIPE);

    // Collision with pipes
    int bx = FB_BIRD_X, by = (int)birdY;
    int bx2 = bx + FB_BIRD_W, by2 = by + FB_BIRD_H;
    int px = pipeX[i], px2 = px + FB_PIPE_W;

    if (bx2 > px && bx < px2) {
      if (by < pipeGapY[i] || by2 > pipeGapY[i] + FB_GAP) {
        fbGameOver = true;
      }
    }
  }

  // Draw bird and score
  drawBird((int)birdY, FB_BIRD);
  drawScore();

  delay(20);
}

GameDef flappyGame = { "Flappy", startFlappy, loopFlappy, 0 };
