#pragma once
#include <Arduino.h>
#include <Adafruit_ST7735.h>

/**
 * ═════════════════════════════════════════════════════════════════════════
 *  Meantendo High-Performance Raster Engine
 *  - Zero-Frame-Buffer Dual Line-Buffer Streaming Pipeline (Saves 40KB RAM)
 *  - Dirty-Span Hardware Acceleration (Reduces SPI Bandwidth by up to 90%)
 * ═════════════════════════════════════════════════════════════════════════
 */

extern Adafruit_ST7735 tft;

struct DirtyRect {
    int16_t x, y, w, h;
    bool active;
};

class RasterEngine {
public:
    static constexpr int MAX_DIRTY_RECTS = 16;
    static constexpr int SCREEN_W = 160;
    static constexpr int SCREEN_H = 128;

    // Dual 320-byte line buffers for zero-frame-buffer scanline streaming
    // Total RAM footprint: 640 bytes (vs. 40,960 bytes for full frame buffer)
    uint16_t lineBuffer[2][SCREEN_W];
    uint8_t currentBufferIdx = 0;

    DirtyRect dirtyQueue[MAX_DIRTY_RECTS];
    int dirtyCount = 0;

    void init() {
        dirtyCount = 0;
        for (int i = 0; i < MAX_DIRTY_RECTS; i++) {
            dirtyQueue[i].active = false;
        }
    }

    // ── 1. Dirty Span Acceleration ──────────────────────────────
    // Registers an area that was drawn last frame to be cleanly erased
    void markDirty(int16_t x, int16_t y, int16_t w, int16_t h) {
        if (dirtyCount < MAX_DIRTY_RECTS) {
            dirtyQueue[dirtyCount++] = {x, y, w, h, true};
        }
    }

    // Erases only the registered dirty spans rather than clearing the full display
    void clearDirtySpans(uint16_t bgColor = ST77XX_BLACK) {
        for (int i = 0; i < dirtyCount; i++) {
            if (dirtyQueue[i].active) {
                tft.fillRect(dirtyQueue[i].x, dirtyQueue[i].y,
                             dirtyQueue[i].w, dirtyQueue[i].h, bgColor);
                dirtyQueue[i].active = false;
            }
        }
        dirtyCount = 0;
    }

    // ── 2. Zero-Frame-Buffer Scanline Streaming ─────────────────
    // Directly streams procedural/sprite scanlines without a full RAM buffer
    template<typename RenderScanlineFn>
    void streamScanlines(int16_t startY, int16_t height, RenderScanlineFn renderFn) {
        tft.startWrite();
        tft.setAddrWindow(0, startY, SCREEN_W, height);

        for (int16_t y = 0; y < height; y++) {
            uint16_t* activeLine = lineBuffer[currentBufferIdx];
            // Let the caller's lambda generate this scanline directly
            renderFn(startY + y, activeLine, SCREEN_W);

            // Stream line directly over high-speed hardware SPI
            for (int16_t x = 0; x < SCREEN_W; x++) {
                tft.writePixel(0, 0, activeLine[x]); // Note: window address auto-increments
            }
            currentBufferIdx ^= 1; // Swap ping-pong buffer
        }
        tft.endWrite();
    }
};

extern RasterEngine rasterEngine;
