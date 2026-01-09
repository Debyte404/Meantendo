# 🛠️ Meantendo Build Guide

Welcome to the official build guide for the **Meantendo** ESP32 gaming console! This document provides detailed instructions on hardware selection, wiring, and assembly.

## 📋 Components List

| Component | Recommendation | Description |
|-----------|----------------|-------------|
| **Microcontroller** | **ESP32 DevKit V1** OR **ESP32-S3** | The brain of the console. We recommend the standard ESP32 WROOM-32 dev board for compatibility, or S3 for better performance. |
| **Display** | **1.8" TFT ST7735** (128x160) | A common, affordable SPI display. Look for the "Red Tab" or "Green Tab" versions. |
| **Buttons** | **6x Tactile Switches** (6x6mm) | Used for D-Pad (Up, Down, Left, Right) and Action buttons (A, B). |
| **Breadboard / PCB** | Perfboard (7x5cm) | For soldering the components permanently. |
| **Wires** | 24-28 AWG Silicone Wire | For connections. |
| **Battery (Optional)** | LiPo 3.7V + TP4056 Charger | For portable power. |
| **Audio (Optional)** | Piezo Buzzer / MAX98357A | For sound effects. |
| **Switch** | Slide Switch | Power On/Off. |

---

## 🔌 Pinout Configuration

The following pin mapping corresponds to the default configuration in `src/config.h` (or relevant configuration files).

### 🖥️ Display (SPI)

| Display Pin | ESP32 Pin | Logic |
|-------------|-----------|-------|
| **VCC**     | 3.3V      | Power |
| **GND**     | GND       | Ground|
| **CS**      | GPIO 5    | Chip Select |
| **RESET**   | GPIO 17   | Reset |
| **A0 / DC** | GPIO 16   | Data/Command |
| **SDA / MOSI**| GPIO 23 | Master Out Slave In (Shared) |
| **SCK / CLK** | GPIO 18 | Clock (Shared) |
| **LED**     | 3.3V      | Backlight |

### 💾 SD Card (Optional)

| Pin | ESP32 Pin | Logic |
|-----|-----------|-------|
| **CS** | GPIO 22 | Chip Select |
| **MOSI** | GPIO 23 | Shared with Display |
| **CLK** | GPIO 18 | Shared with Display |
| **MISO** | GPIO 19 | Master In Slave Out |

### 🎮 Controls (Buttons)

All buttons are configured as `INPUT_PULLUP`. Connect one side to the GPIO and the other side to **GND**.

| Button | ESP32 Pin | Function |
|--------|-----------|----------|
| **UP** (Joy Y) | GPIO 35 | Analog Y |
| **DOWN** (Joy Y)| GPIO 35 | Analog Y |
| **LEFT** (Joy X)| GPIO 34 | Analog X |
| **RIGHT** (Joy X)| GPIO 34 | Analog X |
| **SELECT** | GPIO 21 | Select / Menu (Moved from 19) |
| **A**    | GPIO 32 | Primary Action |
| **B**    | GPIO 33 | Secondary Action |
| **X**    | GPIO 26 | Action X |
| **Y**    | GPIO 27 | Action Y |
| **Back** | GPIO 25 | Back / Escape |

> **Note:** Pin assignments are defined in `components/meantendo_core/include/meantendo_config.h`.

### 🔊 Audio (Optional)

| Component | ESP32 Pin |
|-----------|-----------|
| **Buzzer (+)** | GPIO 15 |
| **Buzzer (-)** | GND |

---

## 🏗️ Assembly Steps

1.  **Prepare the Board**: If using a perfboard, plan your layout. Place the ESP32 in the center or side to allow access to the USB port.
2.  **Mount the Display**: Solder the display header pins. Ensure the screen is centered.
3.  **Wire the SPI Bus**: Connect `SDA` -> `GPIO 23`, `SCK` -> `GPIO 18`, `CS` -> `GPIO 5`, `RES` -> `GPIO 4`, `DC` -> `GPIO 2`.
4.  **Wire the Power**: Connect Display `VCC` and `LED` to ESP32 `3.3V`, and `GND` to `GND`.
5.  **Install Buttons**: Place the 4 navigational buttons in a cross pattern (D-Pad). Place A and B buttons to the right.
6.  **Wire Buttons**: Connect one leg of each button to GND. Connect the other leg to the corresponding GPIO pin (12, 14, 27, 26, 25, 33).
7.  **Sound**: Connect the buzzer to GPIO 15 and GND.
8.  **Check Connections**: Use a multimeter to verify continuity and check for shorts before powering on.

## 🧪 Testing

1.  Connect via USB.
2.  Flash the firmware using PlatformIO.
3.  On boot, you should see the **Meantendo Logo**.
4.  Test each button in the menu.

Happy Building! 🎮
