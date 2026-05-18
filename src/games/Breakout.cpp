#include "../core/Game.hpp"
#include "../core/Input.hpp"
#include <Adafruit_ST7735.h>

extern Adafruit_ST7735 tft;

// ═══════════════════════════════════════════════════════════
//  BREAKOUT — Classic brick-breaker
//  160×128 display
// ═══════════════════════════════════════════════════════════

#define BK_PADDLE_W   28
#define BK_PADDLE_H   4
#define BK_BALL_SZ    3
#define BK_BRICK_W    15
#define BK_BRICK_H    6
#define BK_COLS       10
#define BK_ROWS       5
#define BK_BRICK_GAP  1
#define BK_BRICK_Y0   16

static const uint16_t BRICK_COLORS[BK_ROWS] = {
  0xF800,  // Red
  0xFD20,  // Orange
  0xFFE0,  // Yellow
  0x07E0,  // Green
  0x07FF,  // Cyan
};

static bool bricks[BK_ROWS][BK_COLS];
static int paddleX;
static float ballX, ballY, ballVX, ballVY;
static int bkScore;
static int bkLives;
static bool bkGameOver;
static bool bkWin;
static bool bkLaunched;

static void drawBrick(int r, int c, uint16_t color) {
  int x = c * (BK_BRICK_W + BK_BRICK_GAP) + 3;
  int y = BK_BRICK_Y0 + r * (BK_BRICK_H + BK_BRICK_GAP);
  tft.fillRect(x, y, BK_BRICK_W, BK_BRICK_H, color);
}

static void drawAllBricks() {
  for (int r = 0; r < BK_ROWS; r++)
    for (int c = 0; c < BK_COLS; c++)
      if (bricks[r][c])
        drawBrick(r, c, BRICK_COLORS[r]);
}

static void drawPaddle(uint16_t color) {
  int y = tft.height() - 10;
  tft.fillRect(paddleX, y, BK_PADDLE_W, BK_PADDLE_H, color);
}

static void drawBall(uint16_t color) {
  tft.fillRect((int)ballX, (int)ballY, BK_BALL_SZ, BK_BALL_SZ, color);
}

static void drawHUD() {
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(2, 2);
  tft.printf("S:%-4d", bkScore);
  tft.setCursor(tft.width() - 30, 2);
  tft.printf("L:%d", bkLives);
}

static void resetBall() {
  ballX = paddleX + BK_PADDLE_W / 2.0f;
  ballY = tft.height() - 16;
  ballVX = 1.0f;
  ballVY = -1.2f;
  bkLaunched = false;
}

static bool allBricksCleared() {
  for (int r = 0; r < BK_ROWS; r++)
    for (int c = 0; c < BK_COLS; c++)
      if (bricks[r][c]) return false;
  return true;
}

void startBreakout() {
  randomSeed(esp_random());
  paddleX = tft.width() / 2 - BK_PADDLE_W / 2;
  bkScore = 0;
  bkLives = 3;
  bkGameOver = false;
  bkWin = false;

  for (int r = 0; r < BK_ROWS; r++)
    for (int c = 0; c < BK_COLS; c++)
      bricks[r][c] = true;

  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_MAGENTA);
  tft.setCursor(15, 45);
  tft.print("BREAKOUT");
  delay(600);
  tft.fillScreen(ST77XX_BLACK);

  drawAllBricks();
  resetBall();
  drawPaddle(ST77XX_WHITE);
  drawHUD();
}

void loopBreakout() {
  if (bkGameOver || bkWin) {
    tft.setTextSize(2);
    if (bkWin) {
      tft.setTextColor(ST77XX_GREEN);
      tft.setCursor(20, 40);
      tft.print("YOU WIN!");
    } else {
      tft.setTextColor(ST77XX_RED);
      tft.setCursor(15, 40);
      tft.print("GAME OVER");
    }
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(35, 65);
    tft.printf("Score: %d", bkScore);
    tft.setCursor(30, 82);
    tft.setTextColor(ST77XX_WHITE);
    tft.print("Press BACK");
    delay(400);
    while (!backPressed()) delay(50);
    return;
  }

  // === Input ===
  Direction dir = readJoystickContinuous();
  drawPaddle(ST77XX_BLACK);
  if (dir == DIR_LEFT)  paddleX -= 4;
  if (dir == DIR_RIGHT) paddleX += 4;
  paddleX = constrain(paddleX, 0, tft.width() - BK_PADDLE_W);
  drawPaddle(ST77XX_WHITE);

  // Launch ball
  if (!bkLaunched) {
    drawBall(ST77XX_BLACK);
    ballX = paddleX + BK_PADDLE_W / 2.0f;
    ballY = tft.height() - 16;
    drawBall(ST77XX_YELLOW);
    if (debouncedPress(BTN_A, 200)) {
      bkLaunched = true;
      ballVX = (random(0, 2) == 0) ? 1.0f : -1.0f;
      ballVY = -1.2f;
    }
    delay(15);
    return;
  }

  // === Move ball ===
  drawBall(ST77XX_BLACK);
  ballX += ballVX;
  ballY += ballVY;

  // Wall bounces
  if (ballX <= 0 || ballX >= tft.width() - BK_BALL_SZ) ballVX = -ballVX;
  if (ballY <= 10) ballVY = -ballVY;

  // Ball missed
  if (ballY >= tft.height()) {
    bkLives--;
    if (bkLives <= 0) {
      bkGameOver = true;
      return;
    }
    resetBall();
    drawHUD();
    delay(300);
    return;
  }

  // Paddle collision
  int py = tft.height() - 10;
  if (ballY + BK_BALL_SZ >= py && ballY + BK_BALL_SZ <= py + BK_PADDLE_H + 2) {
    if (ballX + BK_BALL_SZ >= paddleX && ballX <= paddleX + BK_PADDLE_W) {
      ballVY = -abs(ballVY) - 0.05f; // slight speed up
      // Angle based on where ball hits paddle
      float hitPos = (ballX - paddleX) / (float)BK_PADDLE_W; // 0..1
      ballVX = (hitPos - 0.5f) * 4.0f;

      // Prevent perpendicular stuck loops
      if (ballVX > -0.3f && ballVX <= 0.0f) ballVX = -0.3f;
      if (ballVX < 0.3f && ballVX > 0.0f) ballVX = 0.3f;
      
      // Clamp max speed
      if (ballVY < -4.0f) ballVY = -4.0f;
    }
  }

  // Brick collision
  for (int r = 0; r < BK_ROWS; r++) {
    for (int c = 0; c < BK_COLS; c++) {
      if (!bricks[r][c]) continue;
      int bx = c * (BK_BRICK_W + BK_BRICK_GAP) + 3;
      int by = BK_BRICK_Y0 + r * (BK_BRICK_H + BK_BRICK_GAP);

      if (ballX + BK_BALL_SZ > bx && ballX < bx + BK_BRICK_W &&
          ballY + BK_BALL_SZ > by && ballY < by + BK_BRICK_H) {
        bricks[r][c] = false;
        drawBrick(r, c, ST77XX_BLACK);
        ballVY = -ballVY;
        bkScore += (BK_ROWS - r) * 10; // top rows = more points
        drawHUD();

        if (allBricksCleared()) {
          bkWin = true;
          return;
        }
        goto brickDone;
      }
    }
  }
  brickDone:

  drawBall(ST77XX_YELLOW);
  delay(15);
}

GameDef breakoutGame = { "Breakout", startBreakout, loopBreakout, 0 };
