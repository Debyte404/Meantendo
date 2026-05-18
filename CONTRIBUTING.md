# Contributing to Meantendo

Thanks for your interest in contributing! Meantendo is a personal passion project, but contributions are welcome whether it's a new game, a visual demo, performance improvements, or documentation fixes.

---

## How to Contribute

### Bug Reports

Open an issue with:
- A clear description of the bug
- Steps to reproduce
- Expected vs actual behavior
- ESP32 board version and PlatformIO version

### New Games & Demos

Adding a new game is straightforward:

1. Create `src/games/MyGame.cpp` implementing the `GameDef` interface:

```cpp
#include "../core/Game.hpp"

void myGameStart() {
  // Init game state, draw initial frame
}

void myGameLoop() {
  // Game loop body
  // Check backPressed() to exit
}

GameDef myGame = {
  "My Game",        // displayed in menu
  myGameStart,
  myGameLoop,
  0                // category: 0 = game, 1 = visual demo
};
```

2. Add the extern declaration in `src/main.cpp`:

```cpp
extern GameDef myGame;
```

3. Register it in `setup()`:

```cpp
registerGame(&myGame);
```

4. Build with `pio run` and test.

### Code Style

- Header-only helpers (`inline` functions) are fine for simple modules
- Use `#pragma once` for include guards
- Follow the existing naming: `PascalCase` for types, `SCREAMING_SNAKE_CASE` for macros, `camelCase` for functions
- Keep hardware constants in the relevant `.hpp` file, not hardcoded elsewhere

### Pull Requests

1. Fork the repo and create a feature branch from `main`
2. Keep changes focused — one game per PR is ideal
3. Test on actual hardware (simulators won't catch display/SPI issues)
4. Describe what the change does in the PR body

---

## Areas to Contribute

- More retro games (Space Invaders, Pac-Man dots, Frogger, etc.)
- More visual demos
- Better audio (background music, per-game soundtracks)
- Sleep/wake support for battery operation
- Score persistence via EEPROM/SPIFFS
- 3D-printable enclosure CAD files

---

## Getting Help

If you're stuck, open a discussion or issue. Response time varies — this is a hobby project maintained in spare time.