#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "Game.hpp"
#include "Input.hpp"
#include "Buzzer.hpp"

// ═══════════════════════════════════════════════════════════
//  Meantendo Menu System — "Passion Project" Edition
//  Categories, animated cursor, scrollbar, polished colors
// ═══════════════════════════════════════════════════════════

// Color palette
#define MENU_BG       ST77XX_BLACK
#define MENU_HEADER   tft.color565(0, 200, 255)   // Bright cyan
#define MENU_ITEM     ST77XX_WHITE
#define MENU_SEL_BG   tft.color565(255, 200, 0)   // Warm yellow
#define MENU_SEL_TXT  ST77XX_BLACK
#define MENU_CAT_BAR  tft.color565(40, 40, 60)    // Dark blue-grey
#define MENU_CAT_TXT  tft.color565(120, 180, 255)  // Soft blue
#define MENU_FOOTER   tft.color565(60, 60, 60)
#define MENU_SCROLL   tft.color565(100, 100, 120)
#define MENU_BORDER   tft.color565(50, 50, 70)

extern Adafruit_ST7735 tft;
extern GameDef* gameRegistry[MAX_GAMES];
extern int gameCount;

// ── Internal display list ──
// We build a flat list that interleaves category headers and game items
#define MENU_TYPE_HEADER 0
#define MENU_TYPE_ITEM   1

struct MenuItem {
  uint8_t type;       // HEADER or ITEM
  uint8_t gameIndex;  // index into gameRegistry (only for ITEM)
  const char* label;
};

#define MAX_MENU_ITEMS (MAX_GAMES + 4)
static MenuItem menuItems[MAX_MENU_ITEMS];
static int menuItemCount = 0;

static int selectedIndex = 0;  // index into menuItems[]
static int scrollOffset = 0;

#define VISIBLE_ITEMS 5
#define ITEM_HEIGHT   15
#define HEADER_HEIGHT 14
#define LIST_TOP      24
#define LIST_BOTTOM   (tft.height() - 16)

// ── Build display list ──
static void buildMenuList() {
  menuItemCount = 0;

  // Games section
  bool hasGames = false;
  for (int i = 0; i < gameCount; i++)
    if (gameRegistry[i]->category == 0) { hasGames = true; break; }

  if (hasGames) {
    menuItems[menuItemCount++] = { MENU_TYPE_HEADER, 0, "\x10 GAMES" };
    for (int i = 0; i < gameCount; i++) {
      if (gameRegistry[i]->category == 0)
        menuItems[menuItemCount++] = { MENU_TYPE_ITEM, (uint8_t)i, gameRegistry[i]->name };
    }
  }

  // Demos section
  bool hasDemos = false;
  for (int i = 0; i < gameCount; i++)
    if (gameRegistry[i]->category == 1) { hasDemos = true; break; }

  if (hasDemos) {
    menuItems[menuItemCount++] = { MENU_TYPE_HEADER, 0, "\x10 VISUALS" };
    for (int i = 0; i < gameCount; i++) {
      if (gameRegistry[i]->category == 1)
        menuItems[menuItemCount++] = { MENU_TYPE_ITEM, (uint8_t)i, gameRegistry[i]->name };
    }
  }

  // Make sure selectedIndex points to a selectable item
  if (menuItemCount > 0 && menuItems[selectedIndex].type == MENU_TYPE_HEADER) {
    selectedIndex++;
    if (selectedIndex >= menuItemCount) selectedIndex = 0;
  }
}

// ── Drawing ──

static void drawHeader() {
  // Title bar
  tft.fillRect(0, 0, tft.width(), 20, tft.color565(15, 15, 25));
  tft.drawFastHLine(0, 20, tft.width(), MENU_BORDER);

  const char* title = "MEANTENDO";
  tft.setTextSize(1);
  tft.setTextColor(MENU_HEADER);
  int16_t xTitle = (tft.width() - (int)strlen(title) * 6) / 2;
  tft.setCursor(xTitle, 3);
  tft.print(title);

  // Subtle version tag
  tft.setTextSize(1);
  tft.setTextColor(tft.color565(80, 80, 80));
  tft.setCursor(tft.width() - 20, 11);
  tft.print("v1");
}

static void drawFooter() {
  int footerY = tft.height() - 13;
  tft.fillRect(0, footerY, tft.width(), 13, tft.color565(15, 15, 25));
  tft.drawFastHLine(0, footerY, tft.width(), MENU_BORDER);

  const char* footer = "w/ caffeine by Debyte";
  tft.setTextSize(1);
  tft.setTextColor(MENU_FOOTER);
  int16_t xFooter = (tft.width() - (int)strlen(footer) * 6) / 2;
  tft.setCursor(xFooter, footerY + 3);
  tft.print(footer);
}

static void drawScrollbar() {
  if (menuItemCount <= VISIBLE_ITEMS) return;

  int sbX = tft.width() - 4;
  int sbTop = LIST_TOP;
  int sbHeight = LIST_BOTTOM - LIST_TOP;

  // Track
  tft.drawFastVLine(sbX, sbTop, sbHeight, tft.color565(30, 30, 40));

  // Thumb
  int thumbH = max(6, sbHeight * VISIBLE_ITEMS / menuItemCount);
  int thumbY = sbTop + (sbHeight - thumbH) * scrollOffset / max(1, menuItemCount - VISIBLE_ITEMS);
  tft.fillRect(sbX - 1, thumbY, 3, thumbH, MENU_SCROLL);
}

static void drawMenuList() {
  // Clear list area
  tft.fillRect(0, LIST_TOP, tft.width() - 5, LIST_BOTTOM - LIST_TOP, MENU_BG);

  int y = LIST_TOP + 2;

  for (int vi = 0; vi < VISIBLE_ITEMS; vi++) {
    int idx = scrollOffset + vi;
    if (idx >= menuItemCount) break;

    if (menuItems[idx].type == MENU_TYPE_HEADER) {
      // Category header bar
      tft.fillRect(4, y, tft.width() - 12, HEADER_HEIGHT, MENU_CAT_BAR);
      tft.setTextSize(1);
      tft.setTextColor(MENU_CAT_TXT);
      tft.setCursor(8, y + 3);
      tft.print(menuItems[idx].label);
      y += HEADER_HEIGHT + 2;
    } else {
      // Selectable game item
      bool selected = (idx == selectedIndex);

      if (selected) {
        // Rounded highlight bar
        tft.fillRoundRect(6, y, tft.width() - 16, ITEM_HEIGHT, 3, MENU_SEL_BG);
        tft.setTextColor(MENU_SEL_TXT);

        // Animated cursor arrow
        int arrowX = 9;
        tft.setCursor(arrowX, y + 3);
        tft.setTextSize(1);
        tft.print("\x10"); // ► character
      } else {
        tft.setTextColor(MENU_ITEM);
      }

      // Item label — centered-ish with left padding
      tft.setTextSize(1);
      int labelX = selected ? 20 : 14;
      tft.setCursor(labelX, y + 4);
      tft.print(menuItems[idx].label);

      // Category icon on right side
      if (!selected) {
        uint8_t gi = menuItems[idx].gameIndex;
        if (gi < gameCount && gameRegistry[gi]->category == 1) {
          tft.setTextColor(tft.color565(80, 80, 120));
          tft.setCursor(tft.width() - 22, y + 4);
          tft.print("~");  // visual indicator for demos
        }
      }

      y += ITEM_HEIGHT + 1;
    }
  }

  drawScrollbar();
}

// ── Navigation ──

static int findNextSelectable(int from, int delta) {
  int idx = from;
  for (int attempts = 0; attempts < menuItemCount; attempts++) {
    idx = (idx + delta + menuItemCount) % menuItemCount;
    if (menuItems[idx].type == MENU_TYPE_ITEM) return idx;
  }
  return from; // fallback
}

static void ensureVisible() {
  if (menuItemCount <= VISIBLE_ITEMS) {
    scrollOffset = 0;
    return;
  }
  if (selectedIndex < scrollOffset) {
    scrollOffset = selectedIndex;
  }
  if (selectedIndex >= scrollOffset + VISIBLE_ITEMS) {
    scrollOffset = selectedIndex - VISIBLE_ITEMS + 1;
  }
  // Don't scroll past header if possible
  if (scrollOffset > 0 && menuItems[scrollOffset].type == MENU_TYPE_HEADER) {
    // OK — headers at top of view are fine
  }
  scrollOffset = constrain(scrollOffset, 0, max(0, menuItemCount - VISIBLE_ITEMS));
}

static void moveSelection(int delta) {
  if (menuItemCount == 0) return;
  selectedIndex = findNextSelectable(selectedIndex, delta);
  ensureVisible();
  drawMenuList();
  Buzzer::play(Buzzer::Sound::Click);
}

// ── Public API ──

inline void drawMenuFrame() {
  tft.fillScreen(MENU_BG);
  drawHeader();
  drawFooter();
}

inline void drawMenuItems() {
  drawMenuList();
}

inline void handleMenuInput() {
  if (menuItemCount == 0) {
    Buzzer::play(Buzzer::Sound::Error);
    delay(200);
    return;
  }

  Direction dir = readJoystickAutoRepeat(350, 150);

  if (dir == DIR_UP) {
    moveSelection(-1);
  } else if (dir == DIR_DOWN) {
    moveSelection(+1);
  }

  if (selectPressed() || aPressed()) {
    if (menuItems[selectedIndex].type != MENU_TYPE_ITEM) return;

    Buzzer::play(Buzzer::Sound::Confirm);
    uint8_t gi = menuItems[selectedIndex].gameIndex;

    // Transition effect
    for (int i = 0; i < tft.height(); i += 4) {
      tft.drawFastHLine(0, i, tft.width(), ST77XX_BLACK);
      tft.drawFastHLine(0, tft.height() - i, tft.width(), ST77XX_BLACK);
    }
    tft.fillScreen(ST77XX_BLACK);

    gameRegistry[gi]->start();

    while (true) {
      gameRegistry[gi]->loop();
      if (backPressed()) {
        Buzzer::play(Buzzer::Sound::Click);
        delay(150);
        drawMenuFrame();
        drawMenuList();
        break;
      }
    }
  }

  delay(80);
}

inline void initMenu() {
  buildMenuList();

  // Ensure selectedIndex is on a real item
  if (menuItemCount > 0 && menuItems[selectedIndex].type != MENU_TYPE_ITEM) {
    selectedIndex = findNextSelectable(0, 1);
  }
  ensureVisible();

  drawMenuFrame();
  drawMenuList();
}
