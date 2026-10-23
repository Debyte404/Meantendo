#pragma once
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "Game.hpp"
#include "Input.hpp"
#include "Buzzer.hpp"
#include "PowerStateEngine.hpp"
#include "CloudUpdater.hpp"
#include "Config.hpp"

// Forward declaration from BluetoothPad
void stopBluetoothPad();

// ═══════════════════════════════════════════════════════════
//  Meantendo Tabbed Home System — "Switch & GBA" Style
//  Horizontal Left/Right Category Tabs: GAMES | TOOLS | VISUALS
//  Silky-smooth, zero-artifact, category-isolated navigation
// ═══════════════════════════════════════════════════════════

#define MENU_BG       ST77XX_BLACK
#define MENU_HEADER   tft.color565(0, 220, 255)
#define MENU_ITEM     tft.color565(210, 225, 245)
#define MENU_SEL_BG   tft.color565(255, 195, 0)
#define MENU_SEL_TXT  tft.color565(15, 20, 30)
#define MENU_BORDER   tft.color565(40, 48, 70)
#define MENU_TAB_ACT  tft.color565(0, 140, 225)
#define MENU_TAB_INACT tft.color565(20, 25, 38)
#define MENU_SCROLL   tft.color565(0, 180, 240)

extern Adafruit_ST7735 tft;
extern GameDef* gameRegistry[MAX_GAMES];
extern int gameCount;

static uint8_t currentCategory = CAT_GAMES; // 0=GAMES, 1=TOOLS, 2=VISUALS
static const char* categoryNames[NUM_CATEGORIES] = { "GAMES", "TOOLS", "VISUALS" };

// Filtered items currently displayed in active tab
static uint8_t activeGameIndices[MAX_GAMES];
static int activeItemCount = 0;
static int selectedIndex = 0;
static int scrollOffset = 0;

#define VISIBLE_ITEMS 5
#define ITEM_HEIGHT   15
#define LIST_TOP      34
#define LIST_BOTTOM   114

// ── Populate active category items ──
static void buildCategoryList() {
  activeItemCount = 0;
  for (int i = 0; i < gameCount; i++) {
    if (gameRegistry[i]->category == currentCategory) {
      activeGameIndices[activeItemCount++] = (uint8_t)i;
    }
  }

  selectedIndex = 0;
  scrollOffset = 0;
}

// ── Drawing Header & Category Tabs ──
static void drawHeaderAndTabs() {
  // 1. Top status bar (y = 0..16)
  tft.fillRect(0, 0, tft.width(), 16, tft.color565(12, 16, 26));
  tft.drawFastHLine(0, 16, tft.width(), MENU_BORDER);

  tft.setTextSize(1);
  tft.setTextColor(MENU_HEADER);
  tft.setCursor(6, 4);
  tft.print("MEANTENDO");

  tft.setTextColor(tft.color565(110, 140, 185));
  int verLen = (int)strlen(MEANTENDO_VERSION);
  tft.setCursor(tft.width() - (verLen * 6) - 6, 4);
  tft.print(MEANTENDO_VERSION);

  // 2. Category Tab Bar (y = 17..32)
  tft.fillRect(0, 17, tft.width(), 16, tft.color565(15, 18, 30));
  tft.drawFastHLine(0, 33, tft.width(), MENU_BORDER);

  int tabWidths[NUM_CATEGORIES] = { 50, 48, 52 };
  int tabPositions[NUM_CATEGORIES] = { 4, 56, 106 };

  for (int c = 0; c < NUM_CATEGORIES; c++) {
    int tx = tabPositions[c];
    int tw = tabWidths[c];
    bool isActive = (c == currentCategory);

    if (isActive) {
      // Active Tab Pill with bottom neon accent
      tft.fillRoundRect(tx, 18, tw, 13, 2, MENU_TAB_ACT);
      tft.drawFastHLine(tx + 4, 31, tw - 8, tft.color565(0, 255, 240));
      tft.setTextColor(ST77XX_WHITE);
    } else {
      tft.fillRoundRect(tx, 18, tw, 13, 2, MENU_TAB_INACT);
      tft.setTextColor(tft.color565(100, 120, 150));
    }

    tft.setTextSize(1);
    const char* name = categoryNames[c];
    int nameLen = (int)strlen(name);
    int textX = tx + (tw - (nameLen * 6)) / 2;
    tft.setCursor(textX, 21);
    tft.print(name);
  }
}

static void drawFooter() {
  int footerY = tft.height() - 13;
  tft.fillRect(0, footerY, tft.width(), 13, tft.color565(12, 15, 24));
  tft.drawFastHLine(0, footerY, tft.width(), MENU_BORDER);

  const char* footer = "< >:Tab  ^v:Pick  A:Play";
  tft.setTextSize(1);
  tft.setTextColor(tft.color565(120, 160, 210));
  int16_t xFooter = (tft.width() - ((int)strlen(footer) * 6)) / 2;
  tft.setCursor(xFooter, footerY + 3);
  tft.print(footer);
}

static void drawScrollbar() {
  int sbX = tft.width() - 5;
  int sbTop = LIST_TOP + 1;
  int sbHeight = (LIST_BOTTOM - LIST_TOP) - 2;

  // Erase scrollbar channel completely
  tft.fillRect(sbX - 2, sbTop, 6, sbHeight, MENU_BG);

  if (activeItemCount <= VISIBLE_ITEMS) return;

  tft.drawFastVLine(sbX, sbTop, sbHeight, tft.color565(35, 40, 55));

  int thumbH = max(8, sbHeight * VISIBLE_ITEMS / activeItemCount);
  int thumbY = sbTop + (sbHeight - thumbH) * scrollOffset / max(1, activeItemCount - VISIBLE_ITEMS);
  tft.fillRoundRect(sbX - 1, thumbY, 3, thumbH, 1, MENU_SCROLL);
}

static void drawMenuList() {
  // Wipe entire item area cleanly
  tft.fillRect(0, LIST_TOP, tft.width() - 6, (LIST_BOTTOM - LIST_TOP), MENU_BG);

  if (activeItemCount == 0) {
    tft.setTextColor(tft.color565(120, 130, 150));
    tft.setCursor(20, LIST_TOP + 20);
    tft.print("No Apps in Category");
    drawScrollbar();
    return;
  }

  int y = LIST_TOP + 2;

  for (int vi = 0; vi < VISIBLE_ITEMS; vi++) {
    int idx = scrollOffset + vi;
    if (idx >= activeItemCount) break;

    uint8_t gi = activeGameIndices[idx];
    const char* label = gameRegistry[gi]->name;
    bool selected = (idx == selectedIndex);

    if (selected) {
      tft.fillRoundRect(6, y, tft.width() - 18, ITEM_HEIGHT - 1, 2, MENU_SEL_BG);
      tft.setTextColor(MENU_SEL_TXT);
      tft.setCursor(10, y + 3);
      tft.print(">");
      tft.setCursor(20, y + 3);
      tft.print(label);
    } else {
      tft.drawRoundRect(6, y, tft.width() - 18, ITEM_HEIGHT - 1, 2, tft.color565(25, 32, 48));
      tft.setTextColor(MENU_ITEM);
      tft.setCursor(14, y + 3);
      tft.print(label);
    }

    y += ITEM_HEIGHT;
  }

  drawScrollbar();
}

static void ensureVisible() {
  if (activeItemCount <= VISIBLE_ITEMS) {
    scrollOffset = 0;
    return;
  }
  if (selectedIndex < scrollOffset) {
    scrollOffset = selectedIndex;
  }
  if (selectedIndex >= scrollOffset + VISIBLE_ITEMS) {
    scrollOffset = selectedIndex - VISIBLE_ITEMS + 1;
  }
  scrollOffset = constrain(scrollOffset, 0, max(0, activeItemCount - VISIBLE_ITEMS));
}

static void switchCategory(int delta) {
  currentCategory = (currentCategory + delta + NUM_CATEGORIES) % NUM_CATEGORIES;
  buildCategoryList();
  drawHeaderAndTabs();
  drawMenuList();
  Buzzer::play(Buzzer::Sound::Click);
}

static void moveSelection(int delta) {
  if (activeItemCount == 0) return;
  selectedIndex = (selectedIndex + delta + activeItemCount) % activeItemCount;
  ensureVisible();
  drawMenuList();
  Buzzer::play(Buzzer::Sound::Click);
}

// ── Public API ──

inline void drawMenuFrame() {
  tft.fillScreen(MENU_BG);
  drawHeaderAndTabs();
  drawFooter();
}

inline void drawMenuItems() {
  drawMenuList();
}

inline void handleMenuInput() {
  // Sleep Trigger: When in home menu, pressing BACK sleeps console
  if (backPressed()) {
    Buzzer::play(Buzzer::Sound::Click);
    powerEngine.enterLightSleep();
    Buzzer::play(Buzzer::Sound::Confirm);
    drawMenuFrame();
    drawMenuList();
    return;
  }

  // Auto-repeating joystick input
  Direction dir = readJoystickAutoRepeat(350, 150);

  if (dir == DIR_LEFT) {
    switchCategory(-1);
    delay(100);
    return;
  } else if (dir == DIR_RIGHT) {
    switchCategory(+1);
    delay(100);
    return;
  } else if (dir == DIR_UP) {
    moveSelection(-1);
  } else if (dir == DIR_DOWN) {
    moveSelection(+1);
  }

  // Launch App
  if (aPressed() || selectPressed()) {
    if (activeItemCount == 0) return;

    Buzzer::play(Buzzer::Sound::Confirm);
    uint8_t gi = activeGameIndices[selectedIndex];

    // Transition wipe
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
        // Explicitly disconnect Bluetooth and restore ESP-NOW/WiFi
        stopBluetoothPad();
        CloudUpdater::disconnectAndRestoreEspNow();

        drawMenuFrame();
        drawMenuList();
        break;
      }
    }
  }

  delay(60);
}

inline void initMenu() {
  buildCategoryList();
  drawMenuFrame();
  drawMenuList();
}
