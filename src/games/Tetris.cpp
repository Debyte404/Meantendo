#include "../core/Game.hpp"
#include "../core/Input.hpp"
#include <Adafruit_ST7735.h>

extern Adafruit_ST7735 tft;

// ═══════════════════════════════════════════════════════════
//  TETRIS — Classic block-stacking puzzle
//  160×128 display, 6px cells, 10-wide × 20-tall grid
// ═══════════════════════════════════════════════════════════

#define CELL       6
#define BOARD_W    10
#define BOARD_H    20
#define BOARD_X    ((tft.width() - BOARD_W * CELL) / 2)
#define BOARD_Y    (tft.height() - BOARD_H * CELL - 2)

// Colors for each piece type (I=0, O=1, T=2, S=3, Z=4, L=5, J=6)
static const uint16_t PIECE_COLORS[] = {
  0x07FF,  // I — Cyan
  0xFFE0,  // O — Yellow
  0xF81F,  // T — Magenta
  0x07E0,  // S — Green
  0xF800,  // Z — Red
  0xFD20,  // L — Orange
  0x001F,  // J — Blue
};

// Each piece: 4 rotation states, each is 4 (row,col) offsets
// Stored as: pieces[type][rotation][block][row/col]
static const int8_t PIECES[7][4][4][2] = {
  // I
  {{{0,0},{0,1},{0,2},{0,3}}, {{0,0},{1,0},{2,0},{3,0}},
   {{0,0},{0,1},{0,2},{0,3}}, {{0,0},{1,0},{2,0},{3,0}}},
  // O
  {{{0,0},{0,1},{1,0},{1,1}}, {{0,0},{0,1},{1,0},{1,1}},
   {{0,0},{0,1},{1,0},{1,1}}, {{0,0},{0,1},{1,0},{1,1}}},
  // T
  {{{0,0},{0,1},{0,2},{1,1}}, {{0,0},{1,0},{2,0},{1,1}},
   {{1,0},{1,1},{1,2},{0,1}}, {{0,0},{1,0},{2,0},{1,-1}}},
  // S
  {{{0,1},{0,2},{1,0},{1,1}}, {{0,0},{1,0},{1,1},{2,1}},
   {{0,1},{0,2},{1,0},{1,1}}, {{0,0},{1,0},{1,1},{2,1}}},
  // Z
  {{{0,0},{0,1},{1,1},{1,2}}, {{0,1},{1,0},{1,1},{2,0}},
   {{0,0},{0,1},{1,1},{1,2}}, {{0,1},{1,0},{1,1},{2,0}}},
  // L
  {{{0,0},{1,0},{2,0},{2,1}}, {{0,0},{0,1},{0,2},{1,0}},
   {{0,0},{0,1},{1,1},{2,1}}, {{1,0},{1,1},{1,2},{0,2}}},
  // J
  {{{0,1},{1,1},{2,1},{2,0}}, {{0,0},{1,0},{1,1},{1,2}},
   {{0,0},{0,1},{1,0},{2,0}}, {{0,0},{0,1},{0,2},{1,2}}},
};

static uint8_t board[BOARD_H][BOARD_W];
static int curType, curRot, curX, curY;
static int nextType;
static int score;
static int linesCleared;
static unsigned long lastFall;
static int fallSpeed;
static bool gameOver;

static void drawCell(int bx, int by, uint16_t color) {
  int px = BOARD_X + bx * CELL;
  int py = BOARD_Y + by * CELL;
  tft.fillRect(px + 1, py + 1, CELL - 2, CELL - 2, color);
}

static void drawBoard() {
  // Draw border
  tft.drawRect(BOARD_X - 1, BOARD_Y - 1,
               BOARD_W * CELL + 2, BOARD_H * CELL + 2, tft.color565(80,80,80));

  for (int y = 0; y < BOARD_H; y++) {
    for (int x = 0; x < BOARD_W; x++) {
      if (board[y][x]) {
        drawCell(x, y, PIECE_COLORS[board[y][x] - 1]);
      } else {
        int px = BOARD_X + x * CELL;
        int py = BOARD_Y + y * CELL;
        tft.fillRect(px, py, CELL, CELL, ST77XX_BLACK);
      }
    }
  }
}

static void drawPiece(int type, int rot, int px, int py, uint16_t color) {
  for (int i = 0; i < 4; i++) {
    int bx = px + PIECES[type][rot][i][1];
    int by = py + PIECES[type][rot][i][0];
    if (bx >= 0 && bx < BOARD_W && by >= 0 && by < BOARD_H)
      drawCell(bx, by, color);
  }
}

static bool canPlace(int type, int rot, int px, int py) {
  for (int i = 0; i < 4; i++) {
    int bx = px + PIECES[type][rot][i][1];
    int by = py + PIECES[type][rot][i][0];
    if (bx < 0 || bx >= BOARD_W || by >= BOARD_H) return false;
    if (by >= 0 && board[by][bx]) return false;
  }
  return true;
}

static void lockPiece() {
  for (int i = 0; i < 4; i++) {
    int bx = curX + PIECES[curType][curRot][i][1];
    int by = curY + PIECES[curType][curRot][i][0];
    if (by >= 0 && by < BOARD_H && bx >= 0 && bx < BOARD_W)
      board[by][bx] = curType + 1;
  }
}

static void clearLines() {
  int cleared = 0;
  for (int y = BOARD_H - 1; y >= 0; y--) {
    bool full = true;
    for (int x = 0; x < BOARD_W; x++) {
      if (!board[y][x]) { full = false; break; }
    }
    if (full) {
      cleared++;
      for (int yy = y; yy > 0; yy--)
        memcpy(board[yy], board[yy - 1], BOARD_W);
      memset(board[0], 0, BOARD_W);
      y++; // re-check this row
    }
  }
  if (cleared > 0) {
    int pts[] = {0, 100, 300, 500, 800};
    score += pts[cleared];
    linesCleared += cleared;
    fallSpeed = max(80, 500 - linesCleared * 20);
  }
}

static void spawnPiece() {
  curType = nextType;
  nextType = random(0, 7);
  curRot = 0;
  curX = BOARD_W / 2 - 1;
  curY = 0;
  if (!canPlace(curType, curRot, curX, curY)) {
    gameOver = true;
  }
}

static void drawHUD() {
  // Score on the right side
  int hx = BOARD_X + BOARD_W * CELL + 6;
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
  tft.setCursor(hx, BOARD_Y);
  tft.print("SCR");
  tft.setCursor(hx, BOARD_Y + 10);
  tft.printf("%-5d", score);

  tft.setCursor(hx, BOARD_Y + 26);
  tft.print("LNS");
  tft.setCursor(hx, BOARD_Y + 36);
  tft.printf("%-3d", linesCleared);

  // Next piece preview
  tft.setCursor(hx, BOARD_Y + 52);
  tft.print("NXT");
  tft.fillRect(hx, BOARD_Y + 62, CELL * 4, CELL * 4, ST77XX_BLACK);
  for (int i = 0; i < 4; i++) {
    int px = hx + PIECES[nextType][0][i][1] * CELL;
    int py = BOARD_Y + 62 + PIECES[nextType][0][i][0] * CELL;
    tft.fillRect(px + 1, py + 1, CELL - 2, CELL - 2, PIECE_COLORS[nextType]);
  }
}

// === Start ===
void startTetris() {
  randomSeed(esp_random());
  memset(board, 0, sizeof(board));
  score = 0;
  linesCleared = 0;
  fallSpeed = 500;
  gameOver = false;
  nextType = random(0, 7);
  lastFall = millis();

  tft.fillScreen(ST77XX_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(30, 50);
  tft.print("TETRIS");
  delay(600);
  tft.fillScreen(ST77XX_BLACK);

  spawnPiece();
  drawBoard();
  drawHUD();
}

// === Loop ===
void loopTetris() {
  if (gameOver) {
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_RED);
    tft.setCursor(15, 35);
    tft.print("GAME OVER");
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_YELLOW);
    tft.setCursor(30, 60);
    tft.printf("Score: %d", score);
    tft.setCursor(30, 75);
    tft.setTextColor(ST77XX_WHITE);
    tft.print("Press BACK");
    delay(500);
    while (!backPressed()) delay(50);
    return;
  }

  // Input
  Direction dir = readJoystickStateChange();
  if (dir == DIR_LEFT && canPlace(curType, curRot, curX - 1, curY)) {
    drawPiece(curType, curRot, curX, curY, ST77XX_BLACK);
    curX--;
    drawPiece(curType, curRot, curX, curY, PIECE_COLORS[curType]);
  }
  if (dir == DIR_RIGHT && canPlace(curType, curRot, curX + 1, curY)) {
    drawPiece(curType, curRot, curX, curY, ST77XX_BLACK);
    curX++;
    drawPiece(curType, curRot, curX, curY, PIECE_COLORS[curType]);
  }
  if (dir == DIR_DOWN) {
    // Soft drop
    if (canPlace(curType, curRot, curX, curY + 1)) {
      drawPiece(curType, curRot, curX, curY, ST77XX_BLACK);
      curY++;
      drawPiece(curType, curRot, curX, curY, PIECE_COLORS[curType]);
      lastFall = millis();
    }
  }

  // Rotate on A
  if (debouncedPress(BTN_A, 180)) {
    int newRot = (curRot + 1) % 4;
    if (canPlace(curType, newRot, curX, curY)) {
      drawPiece(curType, curRot, curX, curY, ST77XX_BLACK);
      curRot = newRot;
      drawPiece(curType, curRot, curX, curY, PIECE_COLORS[curType]);
    }
  }

  // Gravity
  if (millis() - lastFall >= (unsigned long)fallSpeed) {
    lastFall = millis();
    if (canPlace(curType, curRot, curX, curY + 1)) {
      drawPiece(curType, curRot, curX, curY, ST77XX_BLACK);
      curY++;
      drawPiece(curType, curRot, curX, curY, PIECE_COLORS[curType]);
    } else {
      lockPiece();
      clearLines();
      drawBoard();
      drawHUD();
      spawnPiece();
    }
  }
}

GameDef tetrisGame = { "Tetris", startTetris, loopTetris, 0 };
