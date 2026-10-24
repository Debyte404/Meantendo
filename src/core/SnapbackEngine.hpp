#pragma once
#include <Arduino.h>

/**
 * ═════════════════════════════════════════════════════════════════════════
 *  Meantendo Smart Snapback & Kinematic Conditioning Engine
 *  Eliminates mechanical spring overshoot on analog joysticks with zero lag.
 * ═════════════════════════════════════════════════════════════════════════
 */

class SnapbackAxisFilter {
public:
    int center = 2048;
    int deadzone = 400;
    int largeDeflectionThresh = 480;    // counts away from center (just beyond 400 deadzone)
    int snapbackVelocityThresh = 20;    // counts per ms towards center
    unsigned long reboundWindowMs = 85; // suppression window duration (covers mechanical spring recoil)
    int intentionalThresh = 1200;       // counts away from center to override suppression

    int lastRaw = 2048;
    int filteredVal = 2048;
    unsigned long lastTimeUs = 0;
    int currentVelocity = 0;
    int originSide = 0;                 // -1 = negative, +1 = positive, 0 = neutral
    unsigned long reboundStartTime = 0;
    bool inReboundState = false;
    bool fastReturnActive = false;

    void calibrateCenter(int restingCenter, int deadzoneWidth = 400) {
        center = restingCenter;
        deadzone = deadzoneWidth;
        lastRaw = restingCenter;
        filteredVal = restingCenter;
    }

    int update(int raw, unsigned long nowMs) {
        int delta = raw - center;
        unsigned long nowUs = micros();
        unsigned long dtUs = (lastTimeUs == 0) ? 2000 : (nowUs - lastTimeUs);

        // Recompute velocity when at least 1.5ms has elapsed to avoid microsecond loop division noise
        if (dtUs >= 1500) {
            currentVelocity = (int)(((long)(raw - lastRaw) * 1000L) / (long)dtUs);
            lastRaw = raw;
            lastTimeUs = nowUs;
        }
        int velocity = currentVelocity;

        // 1. Deflection Detection (latched as soon as stick moves past deadzone)
        if (abs(delta) > largeDeflectionThresh) {
            originSide = (delta > 0) ? 1 : -1;
            inReboundState = false;
            fastReturnActive = false;
        }

        // 2. Trajectory Tracking: Are we returning towards center with high velocity?
        if (originSide != 0) {
            bool movingTowardCenter = (originSide == 1 && velocity < -snapbackVelocityThresh) ||
                                      (originSide == -1 && velocity > snapbackVelocityThresh);
            if (movingTowardCenter) {
                fastReturnActive = true;
            }
        }

        // 3. Overshoot Detection: Did we cross center into the opposing quadrant?
        int currentSide = 0;
        if (delta > deadzone) currentSide = 1;
        else if (delta < -deadzone) currentSide = -1;

        if (fastReturnActive && originSide != 0 && currentSide == -originSide) {
            if (!inReboundState) {
                inReboundState = true;
                reboundStartTime = nowMs;
            }
        }

        // 4. Handle Rebound State
        if (inReboundState) {
            if (nowMs - reboundStartTime < reboundWindowMs) {
                // If user applies deliberate high force in opposite direction, pass it immediately
                if (abs(delta) > intentionalThresh) {
                    inReboundState = false;
                    fastReturnActive = false;
                    originSide = currentSide;
                    filteredVal = raw;
                } else {
                    // Suppress mechanical snapback: clamp to neutral center
                    filteredVal = center;
                }
            } else {
                // Rebound window expired
                inReboundState = false;
                fastReturnActive = false;
                originSide = currentSide;
                filteredVal = raw;
            }
        } else {
            // Normal operation (0 latency)
            filteredVal = raw;
        }

        // 5. Settled in deadzone with low velocity
        if (abs(delta) <= deadzone && !inReboundState) {
            if (abs(velocity) < snapbackVelocityThresh) {
                originSide = 0;
                fastReturnActive = false;
            }
        }

        return filteredVal;
    }
};

class SmartSnapbackEngine {
public:
    SnapbackAxisFilter filterX;
    SnapbackAxisFilter filterY;

    void init(int pinX, int pinY) {
        // Sample initial resting position to dynamically calibrate center (X is horizontally inverted for PCB layout)
        long sumX = 0;
        long sumY = 0;
        const int samples = 32;
        for (int i = 0; i < samples; i++) {
            sumX += (4095 - analogRead(pinX));
            sumY += analogRead(pinY);
            delayMicroseconds(500);
        }
        int centerX = (int)(sumX / samples);
        int centerY = (int)(sumY / samples);

        filterX.calibrateCenter(centerX, 400);
        filterY.calibrateCenter(centerY, 400);
    }

    void filter(int rawX, int rawY, int &outX, int &outY) {
        unsigned long now = millis();
        outX = filterX.update(rawX, now);
        outY = filterY.update(rawY, now);
    }
};

extern SmartSnapbackEngine snapbackEngine;
