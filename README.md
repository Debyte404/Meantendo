# 🎮 Meantendo

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![CI Status](https://github.com/Debyte404/Meantendo/actions/workflows/ci.yml/badge.svg)](https://github.com/Debyte404/Meantendo/actions)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-Build-orange)](https://platformio.org/)
[![C++](https://img.shields.io/badge/Language-C++17-blue)](https://isocpp.org/)

**The Ultimate Open Source ESP32 Gaming Console**

Meantendo is a high-performance, feature-rich gaming platform built on the ESP32 ecosystem. It combines nostalgic gaming with modern embedded features like Over-The-Air (OTA) updates, a responsive Web UI, and a robust plugin system.

## ✨ Features

- **🚀 High Performance**: Optimized C++ engine running on ESP32/ESP32-S3 dual-core processors.
- **📡 Advanced OTA System**: 
  - Seamless firmware updates over WiFi.
  - Interactive Web User Interface for management.
  - A/B Partitioning with automatic rollback protection.
- **🎨 Vibrant Graphics**: Driver support for ST7735 and ST7789 displays with high-speed SPI.
- **💾 Save States**: Persistent save data storage using NVS and LittleFS.
- **🎮 Game Library**: Modular game system allowing easy addition of new titles.
- **🔊 Audio Engine**: PWM and I2S audio support for immersive sound effects.

## 🛠️ Hardware Requirements

| Component | Recommendation | Notes |
|-----------|----------------|-------|
| **MCU** | ESP32 or ESP32-S3 | Modules like WROOM-32 or mini dev boards |
| **Display** | 1.8" TFT (ST7735) or 1.3" IPS (ST7789) | SPI Interface required |
| **Input** | 6x Tactile Buttons | D-Pad (Up/Down/Left/Right), A, B |
| **Power** | LiPo Battery + TP4056 | Or USB power for development |
| **Audio** | Piezo Buzzer or MAX98357A | Optional but recommended |

## 🚀 Installation & Setup

### Prerequisites

1. Install [Visual Studio Code](https://code.visualstudio.com/).
2. Install the [PlatformIO Extension](https://platformio.org/install/ide?install=vscode).
3. Clone this repository:

```bash
git clone https://github.com/Debyte404/Meantendo.git
cd Meantendo
```

### Build & Upload

1. Connect your ESP32 device via USB.
2. Open the project in PlatformIO.
3. Select your environment (e.g., `esp32dev`) in the status bar.
4. Click the **Upload** button (➡️) in the PlatformIO toolbar.

```bash
# Or via terminal
pio run -t upload
```

## 📖 Usage

### Controls

- **D-Pad**: Navigation
- **A Button**: Select / Action / Jump
- **B Button**: Back / Cancel / Attack

### Web Interface (OTA)

1. Connect the Meantendo to your WiFi (configure in `src/config.h` or via AP mode).
2. Navigate to `http://meantendo.local` (or the IP address shown on screen).
3. Drag and drop `.bin` firmware files to update wirelessly!

## 🤝 Contributing

We welcome contributions! Whether it's adding a new game, fixing a bug, or improving documentation, please check out our [Contributing Guide](CONTRIBUTING.md).

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

Please review our [Code of Conduct](CODE_OF_CONDUCT.md) before contributing.

## 📜 License

Distributed under the MIT License. See `LICENSE` for more information.

## 👑 Credits & Maintainers

Maintained by **teerthsharma** (Owner of Seal Cult) 🦭

---
*Built with ❤️ by the Meantendo Community*
