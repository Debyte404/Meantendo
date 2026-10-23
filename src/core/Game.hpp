#pragma once
#include <Arduino.h>

#define MAX_GAMES 24

#define CAT_GAMES    0
#define CAT_TOOLS    1
#define CAT_VISUALS  2
#define NUM_CATEGORIES 3

struct GameDef {
  const char* name;
  void (*start)();
  void (*loop)();
  uint8_t category;   // 0 = Games, 1 = Tools, 2 = Visuals
};

// Declare the registry
extern GameDef* gameRegistry[MAX_GAMES];
extern int gameCount;

// Registration helper
inline void registerGame(GameDef* game) {
  if (gameCount < MAX_GAMES)
    gameRegistry[gameCount++] = game;
}
