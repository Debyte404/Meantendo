#include <Arduino.h>
#include "Display.hpp"
#include "Audio.hpp"

// Implement the C bindings declared in Audio.hpp (if they aren't implemented in Audio.cpp)
// and the new C bindings needed for Display (i_video.c).

extern "C" {

// ============================================================================
// Display Bindings (for i_video.c)
// ============================================================================

void Meantendo_InitDisplay(void) {
    // Defined in Display.hpp as inline, so we can call it here.
    displayInit();
}

void Meantendo_FlushDisplay(uint16_t* buffer) {
    // Buffer is 160x100 or 160x128? 
    // i_video.c sends a 160x128 buffer (with 100 lines of data centered).
    // ST7735 pushColor or drawRGBBitmap.
    
    // tft is available here (from Display.hpp)
    tft.drawRGBBitmap(0, 0, buffer, 160, 128);
}

// ============================================================================
// Audio Bindings (Likely implemented in Audio.cpp, but if not, stub here)
// ============================================================================

// If Audio.cpp exists and implements these, we will get linker errors if we define them here.
// I will assume Audio.cpp exists. If it doesn't, I'll need to add them.
// Based on the header having extern "C", it expects them to be implemented somewhere.

}
