#include "../core/Game.hpp"
#include "../core/Input.hpp"
#include "../core/RasterEngine.hpp"
#include "../core/Wireless.hpp"
#include "../core/CryptoJournal.hpp"
#include "../core/PowerStateEngine.hpp"
#include <Adafruit_ST7735.h>

extern Adafruit_ST7735 tft;

// === CONFIG ===
#define COLOR_BG ST77XX_BLACK
#define COLOR_PADDLE ST77XX_WHITE
#define COLOR_BALL ST77XX_YELLOW
#define COLOR_TEXT ST77XX_CYAN

#define PADDLE_W 4
#define PADDLE_H 20
#define BALL_SIZE 4
#define SPEED_X 2
#define SPEED_Y 2

// === STRUCTS ===
struct Paddle { int x, y; };
struct Ball { int x, y, vx, vy; };

// === GLOBAL STATE ===
static Paddle leftPaddle, rightPaddle;
static Ball ball;
static int leftScore = 0, rightScore = 0;
static bool singlePlayer = true;
static bool modeSelected = false;

// === HELPERS ===
inline void resetBall() {
  ball.x = tft.width() / 2 - BALL_SIZE / 2;
  ball.y = tft.height() / 2 - BALL_SIZE / 2;
  ball.vx = (random(0, 2) == 0 ? SPEED_X : -SPEED_X);
  ball.vy = (random(0, 2) == 0 ? SPEED_Y : -SPEED_Y);
}

inline void drawPaddle(Paddle p) {
  tft.fillRect(p.x, p.y, PADDLE_W, PADDLE_H, COLOR_PADDLE);
}

inline void drawBall(Ball b) {
  tft.fillRect(b.x, b.y, BALL_SIZE, BALL_SIZE, COLOR_BALL);
}

inline void drawScore() {
  tft.setTextSize(1);
  tft.setTextColor(COLOR_TEXT, COLOR_BG);
  tft.setCursor(10, 2);
  tft.print(leftScore);
  tft.setCursor(tft.width() - 20, 2);
  tft.print(rightScore);

  if (!singlePlayer) {
    tft.setCursor(tft.width() / 2 - 24, 2);
    tft.setTextColor(MeantendoWireless::isPeerConnected() ? ST77XX_GREEN : ST77XX_ORANGE, COLOR_BG);
    tft.print(MeantendoWireless::isPeerConnected() ? "P2:RF" : "P2:BTN");
  }
}

inline void resetGame() {
  leftScore = rightScore = 0;
  leftPaddle = {5, tft.height() / 2 - PADDLE_H / 2};
  rightPaddle = {tft.width() - PADDLE_W - 5, tft.height() / 2 - PADDLE_H / 2};
  resetBall();
  tft.fillScreen(COLOR_BG);
  rasterEngine.init();
  cryptoJournal.beginSession();
}

// === MODE SELECTION ===
void showModeSelect() {
  tft.fillScreen(COLOR_BG);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setCursor(20, 25);
  tft.print("PONG");
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, 60);
  tft.print("Press A: 1-Player (vs AI)");
  tft.setCursor(10, 80);
  tft.print("Press B: 2-Player (ESP-NOW)");
  tft.setTextColor(tft.color565(140, 140, 160));
  tft.setCursor(10, 105);
  tft.print("P2 connects wirelessly!");

  while (!modeSelected) {
    if (aPressed()) {
      singlePlayer = true;
      modeSelected = true;
    } else if (bPressed()) {
      singlePlayer = false;
      modeSelected = true;
    }
    delay(50);
  }

  resetGame();
}

// === START FUNCTION ===
void startPong() {
  randomSeed(esp_random());
  modeSelected = false;
  showModeSelect();
}

// === LOOP FUNCTION ===
void loopPong() {
  // Erase only previous frame's moving elements via Dirty-Span Engine
  rasterEngine.clearDirtySpans(COLOR_BG);

  // Register previous positions to clean up next frame
  rasterEngine.markDirty(leftPaddle.x, leftPaddle.y, PADDLE_W, PADDLE_H);
  rasterEngine.markDirty(rightPaddle.x, rightPaddle.y, PADDLE_W, PADDLE_H);
  rasterEngine.markDirty(ball.x, ball.y, BALL_SIZE, BALL_SIZE);

  // --- PLAYER 1 Paddle (Local Joystick) ---
  Direction playerDir = readJoystickContinuous();
  if (playerDir == DIR_UP) leftPaddle.y -= 3;
  else if (playerDir == DIR_DOWN) leftPaddle.y += 3;

  // --- PLAYER 2 Paddle (AI vs ESP-NOW Wireless Peer vs Buttons) ---
  if (singlePlayer) {
    if (ball.y + BALL_SIZE / 2 > rightPaddle.y + PADDLE_H / 2 + 2) rightPaddle.y += 2;
    else if (ball.y + BALL_SIZE / 2 < rightPaddle.y + PADDLE_H / 2 - 2) rightPaddle.y -= 2;
  } else {
    // True Wireless Multiplayer via ESP-NOW!
    if (MeantendoWireless::isPeerConnected()) {
      int remoteY = MeantendoWireless::getRemoteJoyY();
      rightPaddle.y = map(remoteY, 0, 4095, 10, tft.height() - PADDLE_H);
    } else {
      if (xPressed()) rightPaddle.y -= 3;
      else if (yPressed()) rightPaddle.y += 3;
    }

    // Broadcast live match state to companion node (ball X/Y, scores, P1 paddle)
    MeantendoWireless::sendGameState(
        1,
        (uint8_t)constrain(ball.x, 0, 255),
        (uint8_t)constrain(ball.y, 0, 255),
        (uint8_t)leftScore,
        (uint8_t)rightScore,
        2048,
        map(leftPaddle.y, 10, tft.height() - PADDLE_H, 0, 4095),
        0
    );
  }

  // Bound paddles
  leftPaddle.y = constrain(leftPaddle.y, 10, tft.height() - PADDLE_H);
  rightPaddle.y = constrain(rightPaddle.y, 10, tft.height() - PADDLE_H);

  // --- Move Ball ---
  ball.x += ball.vx;
  ball.y += ball.vy;

  // Wall collision
  if (ball.y <= 10 || ball.y >= tft.height() - BALL_SIZE) ball.vy *= -1;

  // Paddle collisions
  if (ball.x <= leftPaddle.x + PADDLE_W &&
      ball.y + BALL_SIZE >= leftPaddle.y &&
      ball.y <= leftPaddle.y + PADDLE_H) {
    ball.vx = abs(ball.vx);
  }
  if (ball.x + BALL_SIZE >= rightPaddle.x &&
      ball.y + BALL_SIZE >= rightPaddle.y &&
      ball.y <= rightPaddle.y + PADDLE_H) {
    ball.vx = -abs(ball.vx);
  }

  // Scoring with Cryptographic Journaling
  if (ball.x <= 0) {
    rightScore++;
    cryptoJournal.recordEvent(0x02, 1, rightScore);
    resetBall();
  }
  if (ball.x >= tft.width() - BALL_SIZE) {
    leftScore++;
    cryptoJournal.recordEvent(0x01, 1, leftScore);
    resetBall();
  }

  // Draw everything cleanly
  drawPaddle(leftPaddle);
  drawPaddle(rightPaddle);
  drawBall(ball);
  drawScore();

  delay(15);
}

// === GAME DEF ===
GameDef pongGame = { "Pong", startPong, loopPong, 0 };
