# Meantendo Wiring Guide

This guide provides the pin connections for the Meantendo handheld console based on the **NodeMCU ESP-32S (38-pin)** development board.

## Pinout Summary Table

| Component | Component Pin | ESP32 Pin | Logic Level |
| :--- | :--- | :--- | :--- |
| **TFT Display (ST7735)** | VCC | 3.3V or 5V | Power |
| | GND | GND | Ground |
| | LED (Backlight) | 3.3V | Backlight Power |
| | CS | **GPIO 5** | SPI CS |
| | DC (RS) | **GPIO 16** | Control |
| | RESET | **GPIO 17** | Control |
| | SCK (SCLK) | **GPIO 18** | SPI Clock |
| | SDA (MOSI) | **GPIO 23** | SPI Data |
| **Joystick Module** | +5V / VCC | 3.3V | Power |
| | GND | GND | Ground |
| | VRx | **GPIO 34** | Analog Input |
| | VRy | **GPIO 35** | Analog Input |
| | SW (Push) | **GPIO 19** | Digital Input |
| **Action Buttons** | BTN A | **GPIO 32** | Digital Pullup |
| | BTN B | **GPIO 33** | Digital Pullup |
| | BTN X | **GPIO 26** | Digital Pullup |
| | BTN Y | **GPIO 27** | Digital Pullup |
| | BTN BACK | **GPIO 25** | Digital Pullup |
| **Buzzer** | Signal (+) | **GPIO 15** | PWM Output |
| | GND (-) | GND | Ground |

## Hardware Notes

### 🕹️ Joystick Orientation
The software (`Input.hpp`) assumes a center value of `2048` for the analog sticks. If your joystick axes are swapped or inverted, you can adjust the logic in `readJoystickContinuous()` in `src/core/Input.hpp`.

### 🔘 Button Wiring
Buttons should be wired between the **ESP32 GPIO** and **GND**. The ESP32's internal pull-up resistors are enabled in `initInput()`, so no external resistors are required. A press is recorded when the pin is pulled `LOW`.

### 📺 Display Voltage
Most ST7735 modules have a 3.3V regulator. You can safely power it from the ESP32's `3V3` pin. If using the `5V` (Vin) pin, ensure your module supports that voltage.

### 🔊 Buzzer Type
The code uses `ledcWriteTone` for the buzzer (`Buzzer.cpp`). It is best used with a **Passive** buzzer for various tones, though an **Active** buzzer will still function (but won't be able to change pitch as effectively).
