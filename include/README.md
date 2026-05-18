# Meantendo — Header Files

This directory contains shared header files used across the project.

## Files

| File | Purpose |
|---|---|
| `Game.hpp` | `GameDef` registry struct and `registerGame()` helper — the core interface all games implement |
| `Display.hpp` | TFT pin definitions, SPI initialization, `tft` global instance |
| `Input.hpp` | Joystick and button pin definitions, read functions (continuous, state-change, auto-repeat) |
| `Buzzer.hpp` | Buzzer init and sound effect playback |
| `Menu.hpp` | Menu rendering, navigation, and input handling |
| `Splash.hpp` | Debyte logo bitmap in PROGMEM, fade-in/out animation helpers |
| `adv.hpp` | Advance demo declarations (canvas triangle) |
| `primitive.hpp` | Primitive drawing demo declarations |

## Adding a New Game Header

Games in `src/games/` don't need entries here unless they expose shared types. The `GameDef` interface in `Game.hpp` is the only cross-game contract.