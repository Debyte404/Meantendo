#include "../core/Game.hpp"
#include "../core/Input.hpp"
#include <Adafruit_ST7735.h>

extern Adafruit_ST7735 tft;

// ═══════════════════════════════════════════════════════════
//  MAZE SOLVER — Clean path carving and solving
// ═══════════════════════════════════════════════════════════

#define MZ_CELL   6
#define MZ_W      26    // 160 / 6
#define MZ_H      21    // 128 / 6
#define MZ_OX     ((tft.width() - MZ_W*MZ_CELL)/2)
#define MZ_OY     ((tft.height() - MZ_H*MZ_CELL)/2)

// Palettes
#define MZ_WALL_COL   tft.color565(30, 30, 30) // Dark walls
#define MZ_PATH_COL   tft.color565(200, 200, 200) // Light path
#define MZ_HEAD_COL   ST77XX_MAGENTA 
#define MZ_SOLVE_COL  ST77XX_YELLOW

#define MZ_VISITED  0x10
#define MZ_WALL_N   0x01
#define MZ_WALL_E   0x02
#define MZ_WALL_S   0x04
#define MZ_WALL_W   0x08

static uint8_t maze[MZ_H][MZ_W];
static bool mzSolving;
static bool mzDone;

// Stack for iterative backtracker
static uint8_t stackX[MZ_W * MZ_H];
static uint8_t stackY[MZ_W * MZ_H];
static int stackTop;

// Solve BFS struct
struct Pt { uint8_t x, y; };
static Pt parent[MZ_H][MZ_W];
static uint8_t solveVisited[MZ_H][MZ_W];
static Pt queue[MZ_W * MZ_H];
static int qHead, qTail;

static void removeWall(int x1, int y1, int x2, int y2) {
  if (x2 == x1 + 1) { maze[y1][x1] &= ~MZ_WALL_E; maze[y2][x2] &= ~MZ_WALL_W; }
  if (x2 == x1 - 1) { maze[y1][x1] &= ~MZ_WALL_W; maze[y2][x2] &= ~MZ_WALL_E; }
  if (y2 == y1 + 1) { maze[y1][x1] &= ~MZ_WALL_S; maze[y2][x2] &= ~MZ_WALL_N; }
  if (y2 == y1 - 1) { maze[y1][x1] &= ~MZ_WALL_N; maze[y2][x2] &= ~MZ_WALL_S; }
}

static const int dx[] = {0, 1, 0, -1};
static const int dy[] = {-1, 0, 1, 0};

// Draw a carved cell and its open connections
static void drawCarvedCell(int x, int y, uint16_t color) {
  int px = MZ_OX + x * MZ_CELL;
  int py = MZ_OY + y * MZ_CELL;
  
  // The center node
  tft.fillRect(px + 2, py + 2, MZ_CELL - 3, MZ_CELL - 3, color);
  
  // The corridors extending to neighbors
  if (!(maze[y][x] & MZ_WALL_N)) tft.fillRect(px + 2, py, MZ_CELL - 3, 2, color);
  if (!(maze[y][x] & MZ_WALL_S)) tft.fillRect(px + 2, py + MZ_CELL - 1, MZ_CELL - 3, 1, color);
  if (!(maze[y][x] & MZ_WALL_W)) tft.fillRect(px, py + 2, 2, MZ_CELL - 3, color);
  if (!(maze[y][x] & MZ_WALL_E)) tft.fillRect(px + MZ_CELL - 1, py + 2, 1, MZ_CELL - 3, color);
}

void startMaze() {
  randomSeed(esp_random());
  mzSolving = false; mzDone = false;
  
  tft.fillScreen(MZ_WALL_COL);
  
  for (int y = 0; y < MZ_H; y++)
    for (int x = 0; x < MZ_W; x++)
      maze[y][x] = MZ_WALL_N | MZ_WALL_E | MZ_WALL_S | MZ_WALL_W;
      
  maze[0][0] |= MZ_VISITED;
  stackTop = 1; stackX[0] = 0; stackY[0] = 0;
  
  drawCarvedCell(0, 0, MZ_HEAD_COL);
}

static bool generateStep() {
  if (stackTop <= 0) return false;

  int cx = stackX[stackTop - 1];
  int cy = stackY[stackTop - 1];

  int dirs[4], ndirs = 0;
  for (int d = 0; d < 4; d++) {
    int nx = cx + dx[d], ny = cy + dy[d];
    if (nx >= 0 && nx < MZ_W && ny >= 0 && ny < MZ_H && !(maze[ny][nx] & MZ_VISITED))
      dirs[ndirs++] = d;
  }

  if (ndirs > 0) {
    int d = dirs[random(0, ndirs)];
    int nx = cx + dx[d], ny = cy + dy[d];
    removeWall(cx, cy, nx, ny);
    maze[ny][nx] |= MZ_VISITED;
    
    stackX[stackTop] = nx; stackY[stackTop] = ny; stackTop++;
    
    drawCarvedCell(cx, cy, MZ_PATH_COL);
    drawCarvedCell(nx, ny, MZ_HEAD_COL);
  } else {
    stackTop--;
    drawCarvedCell(cx, cy, MZ_PATH_COL);
  }
  return true;
}

static void traceSolvePath() {
  int cx = MZ_W - 1, cy = MZ_H - 1;
  while(cx != 0 || cy != 0) {
    int px = MZ_OX + cx * MZ_CELL + MZ_CELL/2;
    int py = MZ_OY + cy * MZ_CELL + MZ_CELL/2;
    
    Pt p = parent[cy][cx];
    int pnx = MZ_OX + p.x * MZ_CELL + MZ_CELL/2;
    int pny = MZ_OY + p.y * MZ_CELL + MZ_CELL/2;
    
    // Draw thick yellow line backwards
    for(int i=-1; i<=1; i++) {
      if(px==pnx) tft.drawLine(px+i, py, pnx+i, pny, MZ_SOLVE_COL);
      else        tft.drawLine(px, py+i, pnx, pny+i, MZ_SOLVE_COL);
    }
    
    cx = p.x; cy = p.y;
    delay(20);
  }
}

static bool solveStep() {
  if (qHead >= qTail) return false;

  for (int steps = 0; steps < 2 && qHead < qTail; steps++) {
    Pt cur = queue[qHead++];
    int cx = cur.x, cy = cur.y;
    
    // Fill explored cell with a subtle blue to indicate search progress
    tft.fillRect(MZ_OX + cx*MZ_CELL + 3, MZ_OY + cy*MZ_CELL + 3, MZ_CELL-5, MZ_CELL-5, tft.color565(100, 100, 255));

    if (cx == MZ_W - 1 && cy == MZ_H - 1) {
      traceSolvePath();
      return false;
    }

    for (int d = 0; d < 4; d++) {
      if (maze[cy][cx] & (1 << d)) continue; // Wall is here
      int nx = cx + dx[d], ny = cy + dy[d];
      if (nx >= 0 && nx < MZ_W && ny >= 0 && ny < MZ_H && !solveVisited[ny][nx]) {
        solveVisited[ny][nx] = 1;
        parent[ny][nx] = { (uint8_t)cx, (uint8_t)cy };
        queue[qTail++] = { (uint8_t)nx, (uint8_t)ny };
      }
    }
  }
  return true;
}

void loopMaze() {
  if (mzDone) {
    delay(3000);
    startMaze();
    return;
  }

  if (!mzSolving) {
    // Generate 1 step at a time for smooth visualization
    if (!generateStep()) {
      // Start and End markers
      tft.fillRect(MZ_OX+2, MZ_OY+2, MZ_CELL-3, MZ_CELL-3, ST77XX_GREEN);
      tft.fillRect(MZ_OX+(MZ_W-1)*MZ_CELL+2, MZ_OY+(MZ_H-1)*MZ_CELL+2, MZ_CELL-3, MZ_CELL-3, ST77XX_RED);
      delay(800);
      
      memset(solveVisited, 0, sizeof(solveVisited));
      qHead = 0; qTail = 0;
      solveVisited[0][0] = 1;
      queue[qTail++] = {0, 0};
      
      mzSolving = true;
    }
  } else {
    // Solve phase
    if (!solveStep()) {
      mzDone = true;
    }
  }
  delay(10);
}

GameDef mazeDemo = { "Maze", startMaze, loopMaze, 1 };
