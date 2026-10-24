#pragma once
#include <Arduino.h>
#include "SnapbackEngine.hpp"
#include "Wireless.hpp"

// =======================================================
// === Pin Definitions (adjust for your wiring)
// =======================================================
#define JOY_X_PIN    34
#define JOY_Y_PIN    35

#define BTN_SELECT   19   // Joystick push button (SW)
#define BTN_A        32
#define BTN_B        33
#define BTN_X        26
#define BTN_Y        27
#define BTN_BACK     25

// =======================================================
// === Joystick Calibration & Deadzone
// =======================================================
#define JOY_CENTER   2048
#define JOY_DEADZONE 400

// =======================================================
// === Direction Enum
// =======================================================
enum Direction {
  DIR_NONE,
  DIR_UP,
  DIR_DOWN,
  DIR_LEFT,
  DIR_RIGHT
};

// =======================================================
// === Button Reads
// =======================================================
inline bool isPressed(uint8_t pin) { return digitalRead(pin) == LOW; }

inline bool selectPressed() { return isPressed(BTN_SELECT); }
inline bool aPressed()      { return isPressed(BTN_A); }
inline bool bPressed()      { return isPressed(BTN_B); }
inline bool xPressed()      { return isPressed(BTN_X); }
inline bool yPressed()      { return isPressed(BTN_Y); }
inline bool backPressed()   { return isPressed(BTN_BACK); }

inline uint8_t readButtonsBitmask() {
  uint8_t mask = 0;
  if (digitalRead(BTN_A) == LOW)      mask |= (1 << 0);
  if (digitalRead(BTN_B) == LOW)      mask |= (1 << 1);
  if (digitalRead(BTN_X) == LOW)      mask |= (1 << 2);
  if (digitalRead(BTN_Y) == LOW)      mask |= (1 << 3);
  if (digitalRead(BTN_SELECT) == LOW) mask |= (1 << 4);
  if (digitalRead(BTN_BACK) == LOW)   mask |= (1 << 5);
  return mask;
}

// Rate-limited wireless telemetry streamer (~66 Hz)
inline void broadcastTelemetryIfDue(int x, int y) {
  static unsigned long lastBroadcast = 0;
  unsigned long now = millis();
  if (now - lastBroadcast >= 15) {
    lastBroadcast = now;
    MeantendoWireless::sendTelemetry(x, y, readButtonsBitmask());
  }
}

// =======================================================
// === Optional Debounced Press
// =======================================================
inline bool debouncedPress(uint8_t pin, unsigned long debounceDelay = 150) {
  static unsigned long lastPressTime = 0;
  if (isPressed(pin) && (millis() - lastPressTime > debounceDelay)) {
    lastPressTime = millis();
    return true;
  }
  return false;
}

inline bool selectPressedD() { return debouncedPress(BTN_SELECT); }
inline bool aPressedD()      { return debouncedPress(BTN_A); }
inline bool bPressedD()      { return debouncedPress(BTN_B); }
inline bool xPressedD()      { return debouncedPress(BTN_X); }
inline bool yPressedD()      { return debouncedPress(BTN_Y); }
inline bool backPressedD()   { return debouncedPress(BTN_BACK); }

// =======================================================
// === Input Initialization
// =======================================================
inline void initInput() {
  pinMode(BTN_SELECT, INPUT_PULLUP);
  pinMode(BTN_A, INPUT_PULLUP);
  pinMode(BTN_B, INPUT_PULLUP);
  pinMode(BTN_X, INPUT_PULLUP);
  pinMode(BTN_Y, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);

  // Initialize anti-snapback filter & calibrate resting center
  snapbackEngine.init(JOY_Y_PIN, JOY_X_PIN);
}

// =======================================================
// === Joystick Continuous Read (with Smart Snapback Filter)
// =======================================================
inline Direction readJoystickContinuous() {
  int rawX = 4095 - analogRead(JOY_Y_PIN); // Correct horizontal orientation for PCB
  int rawY = analogRead(JOY_X_PIN);

  int x = rawX;
  int y = rawY;
  snapbackEngine.filter(rawX, rawY, x, y);

  broadcastTelemetryIfDue(x, y);

  int centerX = snapbackEngine.filterX.center;
  int deadzoneX = snapbackEngine.filterX.deadzone;
  int centerY = snapbackEngine.filterY.center;
  int deadzoneY = snapbackEngine.filterY.deadzone;

  if (y < centerY - deadzoneY) return DIR_UP;
  if (y > centerY + deadzoneY) return DIR_DOWN;
  if (x < centerX - deadzoneX) return DIR_LEFT;
  if (x > centerX + deadzoneX) return DIR_RIGHT;
  return DIR_NONE;
}

// =======================================================
// === Joystick Analog Direct Read (for precise movement)
// =======================================================
inline int readJoystickAnalogX() {
  int rawX = 4095 - analogRead(JOY_Y_PIN);
  int rawY = analogRead(JOY_X_PIN);
  int x, y;
  snapbackEngine.filter(rawX, rawY, x, y);
  broadcastTelemetryIfDue(x, y);
  if (abs(x - snapbackEngine.filterX.center) < snapbackEngine.filterX.deadzone) return snapbackEngine.filterX.center;
  return x;
}

inline int readJoystickAnalogY() {
  int rawX = analogRead(JOY_Y_PIN);
  int rawY = analogRead(JOY_X_PIN);
  int x, y;
  snapbackEngine.filter(rawX, rawY, x, y);
  broadcastTelemetryIfDue(x, y);
  if (abs(y - snapbackEngine.filterY.center) < snapbackEngine.filterY.deadzone) return snapbackEngine.filterY.center;
  return y;
}

// =======================================================
// === Joystick State Change Read (single-trigger with Rebound Guard)
// =======================================================
inline Direction readJoystickStateChange() {
  static Direction lastDir = DIR_NONE;
  static Direction prevRegisteredDir = DIR_NONE;
  static unsigned long lastReleaseTime = 0;

  Direction current = readJoystickContinuous();

  if (current != lastDir && current != DIR_NONE) {
    // Check if this is an immediate opposite mechanical rebound upon release
    bool isOpposite = (prevRegisteredDir == DIR_LEFT && current == DIR_RIGHT) ||
                      (prevRegisteredDir == DIR_RIGHT && current == DIR_LEFT) ||
                      (prevRegisteredDir == DIR_UP && current == DIR_DOWN) ||
                      (prevRegisteredDir == DIR_DOWN && current == DIR_UP);

    if (isOpposite && (millis() - lastReleaseTime < 95)) {
      // Suppress transient mechanical rebound: consume glitch without triggering game action
      lastDir = current;
      return DIR_NONE;
    }

    lastDir = current;
    prevRegisteredDir = current;
    return current;
  }

  if (current == DIR_NONE) {
    if (lastDir != DIR_NONE) {
      lastReleaseTime = millis();
    }
    lastDir = DIR_NONE;
  }
  return DIR_NONE;
}

// =======================================================
// === Joystick Auto-Repeat (for menus/snake)
// =======================================================
inline Direction readJoystickAutoRepeat(int initialDelay = 350, int repeatDelay = 120) {
  static Direction lastDir = DIR_NONE;
  static unsigned long lastTime = 0;
  static bool repeating = false;

  Direction current = readJoystickContinuous();

  if (current != DIR_NONE) {
    if (current != lastDir) { // New push
      lastDir = current;
      lastTime = millis();
      repeating = false;
      return current;
    } else { // Holding
      unsigned long limit = repeating ? repeatDelay : initialDelay;
      if (millis() - lastTime > limit) {
        lastTime = millis();
        repeating = true;
        return current;
      }
    }
  } else {
    lastDir = DIR_NONE;
    repeating = false;
  }
  return DIR_NONE;
}
