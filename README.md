# Meantendo

<p align="center">
  <img src="assets/product.jpeg" alt="Meantendo Console" width="600" />
</p>

<p align="center">
  <strong>A DIY ESP32 Handheld Gaming Console & Low-Latency Wireless Arcade System</strong><br>
  <em>13 games. Infinite demos. Smart Snapback Filter. Sub-5ms Wireless Controller. Built from scratch.</em>
</p>

<p align="center">
  <a href="https://www.arduino.cc/"><img src="https://img.shields.io/badge/Framework-Arduino-00979D?style=flat&logo=arduino" alt="Arduino"></a>
  <a href="https://platformio.org/"><img src="https://img.shields.io/badge/Build%20System-PlatformIO-28a745?style=flat&logo=data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAxMDAgMTAwIj48Y2lyY2xlIGN4PSI1MCIgY3k9IjUwIiByPSI0NSIgZmlsbD0iI2ZmZiIvPjxwYXRoIGQ9Ik01MCA1MEMzNyA1MCAyMSAyNiAyMSA0NWMwIDIgMCA0IDQgN2wzMy0zM00zMiA0NWMwLTE3IDE0LTMxIDMxLTMxIiBmaWxsPSJub25lIiBzdHJva2U9IiMwMDAiIHN0cm9rZS13aWR0aD0iNSIvPjwvc3ZnPg==" alt="PlatformIO"></a>
  <a href="https://www.espressif.com/en/products/socs/esp32"><img src="https://img.shields.io/badge/Hardware-ESP32-E7352C?style=flat&logo=espressif" alt="ESP32"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-MIT-yellow?style=flat" alt="MIT License"></a>
</p>

---

## Demo

[![Meantendo Demo](https://img.youtube.com/vi/cb1YR2BpdeY/0.jpg)](https://www.youtube.com/shorts/cb1YR2BpdeY)

---

## Features

- **13 playable games** — Snake, Pong, Tetris, Flappy Bird, Asteroids, Breakout, Conway's Game of Life, and more.
- **Visual demos & tools** — Wireless Pad, Matrix Rain, Kaleidoscope, Maze Solver, Particles, Fractals, Slime Simulator.
- **Smart Snapback Filter Engine** — Zero-lag asymmetric kinematic filtering eliminating mechanical joystick spring overshoot.
- **Sub-5ms ESP-NOW Wireless Telemetry** — Acts as a wireless game controller streaming to companion dongles (ESP32 / ESP32-S2).
- **Companion Web Arcade** — Connect your laptop or tablet to play big-screen games via embedded WebSockets on `http://192.168.4.1/`.
- **Hardware controls** — Analog joystick + 6-button layout (A, B, X, Y, Select, Back).
- **Buzzer audio feedback** — Click, confirm, and error sounds with non-blocking tone management.
- **Animated menu** — Auto-scrolling, category headers, animated cursor, scanline transitions.
- **Flicker-free splash screen** — Fade-in/out Debyte logo on boot.
- **20MHz SPI** — Smooth gameplay on the ST7735 display.

---

## Hardware

| Component | Model | Notes |
|---|---|---|
| Microcontroller | ESP32 Dev Board (38-pin) | 240MHz dual-core |
| Display | 1.8" ST7735 TFT (160×128) | Hardware SPI @ 20MHz |
| Input | Analog joystick + 5 tactile buttons | Conditioned by SmartSnapbackEngine |
| Audio | Passive buzzer | Dynamic LEDC channel attach/detach |
| Companion Node | ESP32 / ESP32-S2 DevKit | Acts as wireless receiver & web server |

See the [Bill of Materials](docs/BOM.md) for full parts list and links.

---

## Getting Started

### Prerequisites

- [PlatformIO Core](https://docs.platformio.org/page/core.html) or [PlatformIO IDE](https://platformio.org/install/ide?install=vscode) (VS Code extension)
- ESP32 toolchain (installed automatically by PlatformIO)
- A USB cable capable of data sync

### Installation & Upload

```bash
# Clone the repository
git clone https://github.com/Debyte404/Meantendo.git
cd Meantendo

# 1. Build and Flash the Meantendo Handheld Console
pio run --target upload

# 2. Monitor console serial output
pio device monitor

# 3. Build and Flash the Companion Receiver (ESP32 or ESP32-S2)
pio run -d companion -e esp32dev --target upload
# or for ESP32-S2:
pio run -d companion -e esp32s2 --target upload
```

> **Note:** Unplug the TFT display's CS wire (GPIO 5) before uploading if programming fails, then reconnect it after flashing.

For full testing instructions and web arcade steps, see [**docs/TESTING_GUIDE.md**](docs/TESTING_GUIDE.md).

---

## Project Structure

```
Meantendo/
├── src/
│   ├── main.cpp                  # Entry: init → splash → menu loop
│   ├── core/                     # Shared engine
│   │   ├── Display.hpp           # TFT SPI init, pin mapping
│   │   ├── Input.hpp             # Joystick + button reads
│   │   ├── SnapbackEngine.hpp    # Smart Snapback kinematic filter
│   │   ├── SnapbackEngine.cpp    # Filter instance definition
│   │   ├── Wireless.hpp          # Sub-5ms ESP-NOW telemetry broadcaster
│   │   ├── Wireless.cpp          # ESP-NOW implementation
│   │   ├── Buzzer.hpp & .cpp     # Audio feedback
│   │   ├── Game.hpp              # GameDef registry
│   │   ├── Menu.hpp              # Categorized menu system
│   │   └── Splash.hpp            # Fade-in/out logo bitmap
│   └── games/                    # 14 Games, Demos & Tools
│       ├── Snake.cpp
│       ├── Pong.cpp
│       ├── Tetris.cpp
│       ├── Breakout.cpp
│       ├── Asteroids.cpp
│       ├── FlappyBird.cpp
│       ├── GameOfLife.cpp
│       ├── WirelessPad.cpp        # Live wireless gamepad HUD
│       ├── Kaleidoscope.cpp
│       ├── MatrixRain.cpp
│       ├── MazeSolver.cpp
│       ├── Particles.cpp
│       ├── Fractals.cpp
│       └── SlimeSim.cpp
├── companion/                    # Companion Bridge & Web Arcade Firmware
│   ├── platformio.ini            # Multi-env (esp32dev / esp32s2)
│   ├── src/
│   │   └── main_companion.cpp    # ESP-NOW Receiver -> Serial + WebSockets
│   └── web/
│       └── index.html            # Web Arcade Platform (live visualizer + space game)
├── assets/                       # Photos and media
├── docs/
│   ├── BOM.md                    # Bill of materials
│   ├── wiring_guide.md           # Pinout and wiring diagram
│   └── TESTING_GUIDE.md          # Step-by-step verification guide
├── platformio.ini                # Console build configuration (C++17)
└── README.md
```

---

## Controls

| Button | Action |
|---|---|
| **Joystick** | Navigate menu / move player (Anti-snapback protected) |
| **A** | Select / confirm / fire |
| **B** | Back / cancel |
| **X / Y** | Reserved / action buttons |
| **Select** | Alternate confirm |
| **Back** | Exit game to menu |

---

## Credits

Built with caffeine by **Debyte**

Libraries used:
- [Adafruit GFX](https://github.com/adafruit/Adafruit-GFX-Library)
- [Adafruit ST7735](https://github.com/adafruit/Adafruit-ST7735-Library)
- [WebSockets by Markus Sattler](https://github.com/Links2004/arduinoWebSockets)
- [Arduino Core for ESP32](https://github.com/espressif/arduino-esp32)

---

## License

MIT License — see [LICENSE](LICENSE)
