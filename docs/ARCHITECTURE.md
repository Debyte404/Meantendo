# 🏗️ System Architecture

This document serves as the **authoritative reference** for the Meantendo ESP32 firmware configuration.

## 🔌 Pinout Configuration

The following pin mappings are derived directly from the source code (`meantendo_config.h` and `input.c`).

### SPI Bus (Shared SPI2_HOST)
> [!NOTE]
> The Display and SD Card share the same SPI bus (pins 18 and 23). The Chip Select (CS) pins are used to select the active device.
> GPIO 19 (MISO) is initialized by the bus but only used by the SD Card.

| Signal | GPIO | Notes |
| :--- | :--- | :--- |
| **MOSI** | 23 | Shared |
| **SCLK** | 18 | Shared |
| **MISO** | 19 | SD Card Data In |

### Display (ST7735)
| Signal | GPIO | Notes |
| :--- | :--- | :--- |
| **CS** | 5 | Chip Select |
| **DC** | 16 | Data/Command |
| **RST** | 17 | Reset |
| **BL** | NC | Backlight (-1 = Always On) |

### SD Card
| Signal | GPIO | Notes |
| :--- | :--- | :--- |
| **CS** | 22 | Chip Select |
| **Data** | 19, 23 | uses Shared SPI |

### Input Controls
> [!IMPORTANT]
> The **Select Button** has been moved to **GPIO 21** to resolve a conflict with the SD Card MISO line.

| Button | GPIO | Config |
| :--- | :--- | :--- |
| **Joy X** | 34 | ADC1_CH6 |
| **Joy Y** | 35 | ADC1_CH7 |
| **Select** | 21 | Moved from 19 |
| **A** | 32 | |
| **B** | 33 | |
| **X** | 26 | |
| **Y** | 27 | |
| **Back** | 25 | |

## 🐛 Known Issues & Bugs

### 1. PSRAM Requirement
The `DOOM` port requires PSRAM. Standard ESP32 WROOM modules without PSRAM cannot run the game.

### 2. Display Color Inversion
ST7735 displays come in "Red Tab" and "Green Tab" variants. If colors look inverted (Blue is Red), you may need to adjust the `MADCTL` register settings in `components/meantendo_display/src/display.c`.

## 🛠️ Debugging Tips

### Enable Serial Debugging
To view detailed logs, ensure `MEANTENDO_DEBUG` is set to `1` in `meantendo_config.h`.

```c
#define MEANTENDO_DEBUG 1
```

### Monitor Output
Use the ESP-IDF monitor tool to see runtime logs and potential crashes/panics.

```bash
idf.py monitor
```
