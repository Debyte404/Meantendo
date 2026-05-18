#pragma once
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// === PINS ===
#define TFT_CS   5   // REMINDER: Unplug this wire during upload!
#define TFT_DC   16
#define TFT_RST  17
#define TFT_SCLK 18
#define TFT_MOSI 23

// Fixed: Use standard SPI, not 'vspi'
inline Adafruit_ST7735 tft(TFT_CS, TFT_DC, TFT_RST);

inline void displayInit() {
  // 1. Start standard SPI
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);

  // 2. Init Display
  // If colors look weird (blue is red), switch to INITR_BLACKTAB
  tft.initR(INITR_GREENTAB); 
  
  // 3. Landscape Inverted (matches wiring orientation)
  tft.setRotation(3); 

  // 4. Clear screen
  tft.fillScreen(ST77XX_BLACK); 
  
  // 5. Crank SPI speed for smooth gameplay
  tft.setSPISpeed(20000000); 
}