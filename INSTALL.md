# Meantendo — Installation Guide

This guide walks you through setting up the development environment and flashing Meantendo onto your ESP32 dev board.

---

## Prerequisites

| Requirement | Version | Notes |
|---|---|---|
| [PlatformIO Core](https://docs.platformio.org/page/core.html) | Latest | CLI tool, or |
| [PlatformIO IDE](https://platformio.org/install/ide?install=vscode) | Latest | VS Code extension |
| [Python](https://www.python.org/) | 3.8+ | Required by PlatformIO |
| ESP32 board package | Latest | Auto-installed by PlatformIO |

---

## Step 1 — Install PlatformIO

### Option A: VS Code Extension (Recommended)

1. Install [VS Code](https://code.visualstudio.com/)
2. Open VS Code and go to Extensions (`Ctrl+Shift+X`)
3. Search for **PlatformIO IDE** and click Install
4. Restart VS Code when prompted

### Option B: CLI Only

```bash
pip install -U platformio
# or via homebrew (macOS/Linux)
brew install platformio
```

---

## Step 2 — Configure the COM Port

Open `platformio.ini` and set `upload_port` to your ESP32's COM port:

```ini
[env:esp32dev]
upload_port = COM5    ; ← change to your port (e.g. COM3, /dev/ttyUSB0)
```

To find the COM port:
- **Windows:** Device Manager → Ports (COM & LPT) → look for "USB Serial Device"
- **Linux:** `ls /dev/ttyUSB*` or `ls /dev/ttyACM*`
- **macOS:** `ls /dev/tty.*` — look for `cu.usbserial-*` or `cu.SLAB_USBtoUART`

---

## Step 3 — Wire the Hardware

Refer to [docs/wiring_guide.md](docs/wiring_guide.md) for the full pinout and wiring diagram.

> **Important:** Unplug GPIO 5 (TFT CS) before uploading. Reconnect it after flashing.

---

## Step 4 — Build

```bash
pio run
```

First run takes 2–5 minutes as it downloads the ESP32 toolchain.

---

## Step 5 — Upload

```bash
pio run --target upload
```

Wait for completion. You should see:
```
Hard resetting via RTS pin...
========== [SUCCESS] Took 7.42 seconds ==========
```

---

## Step 6 — Reconnect the Display

After upload, reconnect GPIO 5 (TFT CS) if removed, then power cycle the ESP32. The splash screen should fade in followed by the game menu.

---

## Step 7 — Monitor Serial Output

```bash
pio device monitor
```

Debug output at 115200 baud.

---

## Troubleshooting

### Upload fails with "Failed to connect"
1. Disconnect GPIO 5 (TFT CS) during upload
2. Hold BOOT button on ESP32, press RST, release both
3. Try a different USB cable
4. Lower `upload_speed` in `platformio.ini` to 460800

### Display shows white screen
- Check all 8 wiring connections carefully
- Try a different 3.3V pin or power source
- Verify `tft.initR(INITR_GREENTAB)` matches your display variant

### Joystick unresponsive
- Swap X and Y pin assignments in `Input.hpp` (pins 34 and 35)
- Calibrate: check `JOY_CENTER` value matches your joystick's actual rest value

---

## Next Steps

- Play through all 13 games!
- Try [adding a new game](CONTRIBUTING.md#new-games--demos)
- Upgrade to a Li-Po battery for true handheld operation
- Design a 3D-printed enclosure