#include "../core/Game.hpp"
#include "../core/Input.hpp"
#include <Adafruit_ST7735.h>

extern Adafruit_ST7735 tft;

// ═══════════════════════════════════════════════════════════
//  GAME OF LIFE — Conway's Cellular Automata
// ═══════════════════════════════════════════════════════════

#define GOL_W 40
#define GOL_H 32
#define CELL_S 4

static uint8_t *golGrid = nullptr;
static uint8_t *golNext = nullptr;

static int cursorX, cursorY;
static bool playing;
static bool needRedraw;

static void randomizeGrid() {
  for (int y = 0; y < GOL_H; y++) {
    for (int x = 0; x < GOL_W; x++) {
      golGrid[y * GOL_W + x] = (random(0, 100) < 20) ? 1 : 0; // 20% fill
    }
  }
  needRedraw = true;
}

static void clearGrid() {
  memset(golGrid, 0, GOL_W * GOL_H);
  needRedraw = true;
}

static int countNeighbors(int cx, int cy) {
  int count = 0;
  for (int dy = -1; dy <= 1; dy++) {
    for (int dx = -1; dx <= 1; dx++) {
      if (dx == 0 && dy == 0) continue;
      int nx = cx + dx;
      int ny = cy + dy;
      if (nx < 0) nx += GOL_W; else if (nx >= GOL_W) nx -= GOL_W; // Wrap X
      if (ny < 0) ny += GOL_H; else if (ny >= GOL_H) ny -= GOL_H; // Wrap Y
      if (golGrid[ny * GOL_W + nx]) count++;
    }
  }
  return count;
}

static void stepGeneration() {
  for (int y = 0; y < GOL_H; y++) {
    for (int x = 0; x < GOL_W; x++) {
      int neighbors = countNeighbors(x, y);
      int alive = golGrid[y * GOL_W + x];
      
      if (alive) {
        if (neighbors < 2 || neighbors > 3) golNext[y * GOL_W + x] = 0;
        else golNext[y * GOL_W + x] = 1;
      } else {
        if (neighbors == 3) golNext[y * GOL_W + x] = 1;
        else golNext[y * GOL_W + x] = 0;
      }
    }
  }
  
  // Swap
  uint8_t *tmp = golGrid;
  golGrid = golNext;
  golNext = tmp;
  needRedraw = true;
}

static void drawGridFull() {
  for (int y = 0; y < GOL_H; y++) {
    for (int x = 0; x < GOL_W; x++) {
      uint16_t color = golGrid[y * GOL_W + x] ? ST77XX_CYAN : ST77XX_BLACK;
      tft.fillRect(x * CELL_S, y * CELL_S, CELL_S - 1, CELL_S - 1, color);
    }
  }
  needRedraw = false;
}

void startGameOfLife() {
  if (!golGrid) golGrid = (uint8_t*)malloc(GOL_W * GOL_H);
  if (!golNext) golNext = (uint8_t*)malloc(GOL_W * GOL_H);
  
  cursorX = GOL_W / 2;
  cursorY = GOL_H / 2;
  playing = false;
  
  clearGrid();
  tft.fillScreen(ST77XX_BLACK);
  
  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(1);
  tft.setCursor(20, 40);
  tft.print("GAME OF LIFE");
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(10, 60); tft.print("A: Toggle Cell");
  tft.setCursor(10, 75); tft.print("B: Play/Pause");
  tft.setCursor(10, 90); tft.print("X: Step  Y: Clear");
  tft.setCursor(10, 105); tft.print("SELECT: Random");
  delay(2000);
  
  tft.fillScreen(ST77XX_BLACK);
  needRedraw = true;
}

void loopGameOfLife() {
  if (!golGrid || !golNext) { delay(100); return; } // Memory fail safeguard

  // ── Input Handling ──
  Direction dir = readJoystickAutoRepeat(150, 60);
  if (dir != DIR_NONE) {
    // Erase old cursor outline
    tft.drawRect(cursorX * CELL_S - 1, cursorY * CELL_S - 1, CELL_S + 1, CELL_S + 1, ST77XX_BLACK);

    if (dir == DIR_LEFT)  cursorX--;
    if (dir == DIR_RIGHT) cursorX++;
    if (dir == DIR_UP)    cursorY--;
    if (dir == DIR_DOWN)  cursorY++;

    if (cursorX < 0) cursorX += GOL_W; if (cursorX >= GOL_W) cursorX -= GOL_W;
    if (cursorY < 0) cursorY += GOL_H; if (cursorY >= GOL_H) cursorY -= GOL_H;
  }

  if (debouncedPress(BTN_A, 150)) {
    int idx = cursorY * GOL_W + cursorX;
    golGrid[idx] = !golGrid[idx];
    tft.fillRect(cursorX * CELL_S, cursorY * CELL_S, CELL_S - 1, CELL_S - 1, golGrid[idx] ? ST77XX_CYAN : ST77XX_BLACK);
  }

  if (debouncedPress(BTN_B, 300)) {
    playing = !playing;
    // visual feedback top corner
    tft.fillRect(0,0, 6, 6, playing ? ST77XX_GREEN : ST77XX_RED);
    delay(50);
    tft.fillRect(0,0, 6, 6, golGrid[0] ? ST77XX_CYAN : ST77XX_BLACK);
  }

  if (!playing && debouncedPress(BTN_X, 150)) {
    stepGeneration();
  }

  if (debouncedPress(BTN_Y, 400)) {
    clearGrid();
    playing = false;
  }

  if (debouncedPress(BTN_SELECT, 400)) {
    randomizeGrid();
  }

  // ── Simulation Step ──
  static unsigned long lastStep = 0;
  if (playing && millis() - lastStep > 100) {
    stepGeneration();
    lastStep = millis();
  }

  // ── Draw ──
  if (needRedraw) { // Full redraw triggered by clear or random
    drawGridFull();
  } else if (playing) { // Diff-based redraw
    for (int y = 0; y < GOL_H; y++) {
      for (int x = 0; x < GOL_W; x++) {
        // Since we swapped grids, compare golGrid with golNext to see what changed
        // Wait, golNext now holds the old state from last step!
        if (golGrid[y * GOL_W + x] != golNext[y * GOL_W + x]) {
          uint16_t color = golGrid[y * GOL_W + x] ? ST77XX_CYAN : ST77XX_BLACK;
          tft.fillRect(x * CELL_S, y * CELL_S, CELL_S - 1, CELL_S - 1, color);
        }
      }
    }
  }

  // ── Draw Cursor Outline ──
  // Permanent visible outline that rests exclusively in the 1px grid gap.
  tft.drawRect(cursorX * CELL_S - 1, cursorY * CELL_S - 1, CELL_S + 1, CELL_S + 1, ST77XX_YELLOW);

  delay(10);
}

GameDef conwayGame = { "Life (Sim)", startGameOfLife, loopGameOfLife, 0 };
