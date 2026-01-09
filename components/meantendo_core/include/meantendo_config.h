/**
 * @file meantendo_config.h
 * @brief Meantendo Gaming Console - Global Configuration
 * @author Debyte
 * @version 2.0.0 (ESP-IDF Native)
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
//  Version Information
// ============================================================================
#define MEANTENDO_VERSION_MAJOR     2
#define MEANTENDO_VERSION_MINOR     0
#define MEANTENDO_VERSION_PATCH     0
#define MEANTENDO_VERSION_STRING    "2.0.0"
#define MEANTENDO_CODENAME          "DOOM-Ready"

// ============================================================================
//  Display Configuration (ST7735 160x128)
// ============================================================================
#define DISPLAY_WIDTH           160
#define DISPLAY_HEIGHT          128
#define DISPLAY_ROTATION        1       // Landscape mode

// SPI Pins
#define DISPLAY_PIN_MOSI        23
#define DISPLAY_PIN_SCLK        18
#define DISPLAY_PIN_CS          5
#define DISPLAY_PIN_DC          16
#define DISPLAY_PIN_RST         17
#define DISPLAY_PIN_BL          -1      // Backlight pin (-1 = always on)

// SD Card Configuration
#define SD_SPI_HOST             SPI2_HOST // Shared with Display
#define SD_MOUNT_POINT          "/sd"

// SD Card Pins (Shared SPI Bus)
#define SD_PIN_MISO             19
#define SD_PIN_MOSI             23
#define SD_PIN_CLK              18
#define SD_PIN_CS               22    // Separate CS for SD

// SPI Settings
#define DISPLAY_SPI_HOST        SPI2_HOST
#define DISPLAY_SPI_FREQ_HZ     40000000    // 40 MHz
#define DISPLAY_QUEUE_SIZE      7

// ============================================================================
//  Input Configuration
// ============================================================================

// Joystick ADC Pins
#define INPUT_JOY_X_PIN         34
#define INPUT_JOY_Y_PIN         35

// Button GPIOs
#define INPUT_BTN_SELECT        21    // Moved from 19 to avoid conflict with VSPI MISO
#define INPUT_BTN_A             32
#define INPUT_BTN_B             33
#define INPUT_BTN_X             26
#define INPUT_BTN_Y             27
#define INPUT_BTN_BACK          25

// Joystick Calibration
#define INPUT_JOY_CENTER        2048
#define INPUT_JOY_DEADZONE      400
#define INPUT_DEBOUNCE_MS       50

// ============================================================================
//  Game System Configuration
// ============================================================================
#define MAX_GAMES               16
#define MAX_GAME_NAME_LEN       32
#define GAME_LOOP_DELAY_MS      10

// Menu Configuration
#define MENU_VISIBLE_ITEMS      4
#define MENU_ITEM_HEIGHT        17
#define MENU_START_Y            35

// ============================================================================
//  Color Palette (RGB565)
// ============================================================================
#define COLOR_BLACK             0x0000
#define COLOR_WHITE             0xFFFF
#define COLOR_RED               0xF800
#define COLOR_GREEN             0x07E0
#define COLOR_BLUE              0x001F
#define COLOR_YELLOW            0xFFE0
#define COLOR_CYAN              0x07FF
#define COLOR_MAGENTA           0xF81F
#define COLOR_ORANGE            0xFD20
#define COLOR_DARK_GREY         0x4208

// Meantendo Brand Colors
#define COLOR_BRAND_PRIMARY     0xFFE0      // Yellow
#define COLOR_BRAND_SECONDARY   0x07FF      // Cyan
#define COLOR_BRAND_ACCENT      0xF800      // Red

// ============================================================================
//  OTA Configuration
// ============================================================================
#define OTA_GITHUB_OWNER        "Debyte404"
#define OTA_GITHUB_REPO         "Meantendo"
#define OTA_GITHUB_API_URL      "https://api.github.com/repos/" OTA_GITHUB_OWNER "/" OTA_GITHUB_REPO "/releases/latest"
#define OTA_TIMEOUT_MS          30000
#define OTA_RETRY_COUNT         3

// WiFi AP Mode (for initial setup)
#define OTA_AP_SSID             "Meantendo-Setup"
#define OTA_AP_PASS             "meantendo123"

// ============================================================================
//  Memory Configuration
// ============================================================================
#ifdef CONFIG_SPIRAM
    #define MEANTENDO_HAS_PSRAM     1
    #define DOOM_SUPPORTED          1
#else
    #define MEANTENDO_HAS_PSRAM     0
    #define DOOM_SUPPORTED          0
#endif

// Game framebuffer settings
#define FRAMEBUFFER_IN_PSRAM    MEANTENDO_HAS_PSRAM
#define FRAMEBUFFER_SIZE        (DISPLAY_WIDTH * DISPLAY_HEIGHT * 2)   // RGB565

// ============================================================================
//  Debug Configuration
// ============================================================================
#ifndef MEANTENDO_DEBUG
    #define MEANTENDO_DEBUG         0
#endif

#if MEANTENDO_DEBUG
    #define DEBUG_PRINT(...)        printf(__VA_ARGS__)
#else
    #define DEBUG_PRINT(...)        ((void)0)
#endif

#ifdef __cplusplus
}
#endif
