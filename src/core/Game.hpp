#pragma once
#include <Arduino.h>

#define MAX_GAMES 20

struct GameDef {
  const char* name;
  void (*start)();
  void (*loop)();
  uint8_t category;   // 0 = Game, 1 = Visual Demo
};

// Declare the registry
extern GameDef* gameRegistry[MAX_GAMES];
extern int gameCount;

// Simple manual registration helper
inline void registerGame(GameDef* game) {
  if (gameCount < MAX_GAMES)
    gameRegistry[gameCount++] = game;
}
