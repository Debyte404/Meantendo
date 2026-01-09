# 🎮 Meantendo

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![CI Status](https://github.com/Debyte404/Meantendo/actions/workflows/ci.yml/badge.svg)](https://github.com/Debyte404/Meantendo/actions)
[![PlatformIO](https://img.shields.io/badge/PlatformIO-Build-orange)](https://platformio.org/)
[![C++](https://img.shields.io/badge/Language-C++17-blue)](https://isocpp.org/)

**The Ultimate Open Source ESP32 Gaming Console**

Meantendo is a high-performance, feature-rich gaming platform built on the ESP32 ecosystem. It combines nostalgic gaming with modern embedded features like **Advanced Over-The-Air (OTA) updates**, a beautiful **Web UI**, and a robust **Component-based Architecture**.

## ✨ Key Features

- **🚀 High Performance**: Optimized C++ engine running on ESP32/ESP32-S3 dual-core processors.
- **📡 Hyper-Advanced OTA System**: 
  - **GitHub Releases Integration**: Automatically fetch updates from your repo.
  - **Web Dashboard**: Manage firmware via a beautiful responsive Web UI (`src/ota/README.md`).
  - **Safe Rollbacks**: A/B Partitioning ensures you never brick your device.
- **🧱 Component Architecture**: Modular design separating Core, Display, Input, and Games.
- **😈 Native DOOM Port**: Experimental support for running DOOM as a native ESP-IDF component.
- **🎨 Vibrant Graphics**: Driver support for ST7735 and ST7789 displays with high-speed SPI.
- **💾 Save States**: Persistent save data storage using NVS and LittleFS.
- **🤖 Robust CI/CD**: Automated builds, testing, and release generation via GitHub Actions.

## 🕹️ Supported Games

- **� Snake**: The classic arcade game.
- **🏓 Pong**: Two-player competitive paddle game.
- **💀 DOOM**: (Experimental) The legendary FPS running natively.

## 🛠️ Hardware & Build Guide

Want to build your own? We have a comprehensive guide for you!

👉 **[Read the Official Build Guide](docs/BUILD_GUIDE.md)**

**Quick Specs:**
- **MCU**: ESP32 or ESP32-S3
- **Display**: 1.8" TFT (ST7735)
- **Input**: 6 Tactile Buttons

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
3. Select your environment (e.g., `esp32dev` or `esp32s3`).
4. **Upload Filesystem Image** (Important for Web UI assets!): `PlatformIO > Project Tasks > Platform > Upload Filesystem Image`.
5. Click **Upload** (➡️) to flash the firmware.

```bash
# Or via terminal
pio run -t uploadfs
pio run -t upload
```

## 📖 Usage

### Controls

- **D-Pad**: Navigation
- **A Button**: Select / Action / Jump
- **B Button**: Back / Cancel / Attack

### Web Interface (OTA)

1. Connect the Meantendo to your WiFi (configure via the "WiFi Config" menu item).
2. Navigate to `http://meantendo.local` (or the IP address shown on screen).
3. Drag and drop `.bin` firmware files or check for online updates!

## 🤝 Contributing

We welcome contributions! Please check out our [Contributing Guide](CONTRIBUTING.md).

1. Fork the Project
2. Create your Feature Branch (`git checkout -b feature/AmazingFeature`)
3. Commit your Changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the Branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

## 📜 License

Distributed under the MIT License. See `LICENSE` for more information.

## 👑 Credits & Maintainers

Maintained by **teerthsharma** (Owner of Seal Cult) 🦭

---
*Built with ❤️ by the Meantendo Community*
