/**
 * @file display.h
 * @brief Meantendo Display Driver - ST7735 TFT
 * @author Debyte
 * @version 2.0.0 (ESP-IDF Native)
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
//  Display Initialization
// ============================================================================

/**
 * @brief Initialize the ST7735 display
 * @return ESP_OK on success
 */
esp_err_t display_init(void);

/**
 * @brief Deinitialize the display
 */
void display_deinit(void);

// ============================================================================
//  Basic Drawing Functions
// ============================================================================

/**
 * @brief Fill entire screen with a color
 * @param color RGB565 color
 */
void display_fill_screen(uint16_t color);

/**
 * @brief Draw a single pixel
 * @param x X coordinate
 * @param y Y coordinate
 * @param color RGB565 color
 */
void display_draw_pixel(int16_t x, int16_t y, uint16_t color);

/**
 * @brief Draw a filled rectangle
 * @param x X position
 * @param y Y position
 * @param w Width
 * @param h Height
 * @param color RGB565 color
 */
void display_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

/**
 * @brief Draw a rectangle outline
 * @param x X position
 * @param y Y position
 * @param w Width
 * @param h Height
 * @param color RGB565 color
 */
void display_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

/**
 * @brief Draw a horizontal line
 * @param x Start X
 * @param y Y position
 * @param w Width
 * @param color RGB565 color
 */
void display_draw_hline(int16_t x, int16_t y, int16_t w, uint16_t color);

/**
 * @brief Draw a vertical line
 * @param x X position
 * @param y Start Y
 * @param h Height
 * @param color RGB565 color
 */
void display_draw_vline(int16_t x, int16_t y, int16_t h, uint16_t color);

/**
 * @brief Draw a line between two points
 * @param x0 Start X
 * @param y0 Start Y
 * @param x1 End X
 * @param y1 End Y
 * @param color RGB565 color
 */
void display_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);

/**
 * @brief Draw a circle outline
 */
void display_draw_circle(int16_t x, int16_t y, int16_t r, uint16_t color);

/**
 * @brief Draw a filled circle
 */
void display_fill_circle(int16_t x, int16_t y, int16_t r, uint16_t color);

// ============================================================================
//  Bitmap Drawing
// ============================================================================

/**
 * @brief Draw a 1-bit bitmap (vertical byte order)
 * @param x X position
 * @param y Y position
 * @param bitmap Bitmap data
 * @param w Width
 * @param h Height
 * @param color Foreground color
 */
void display_draw_bitmap_vertical(int16_t x, int16_t y, const uint8_t *bitmap, 
                                   int16_t w, int16_t h, uint16_t color);

/**
 * @brief Draw an RGB565 bitmap
 * @param x X position
 * @param y Y position
 * @param bitmap RGB565 pixel data
 * @param w Width
 * @param h Height
 */
void display_draw_rgb565(int16_t x, int16_t y, const uint16_t *bitmap, int16_t w, int16_t h);

/**
 * @brief Draw a framebuffer region (for games/DOOM)
 * @param x X position
 * @param y Y position
 * @param w Width
 * @param h Height
 * @param data RGB565 pixel data
 */
void display_blit(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *data);

// ============================================================================
//  Text Drawing
// ============================================================================

/**
 * @brief Set text cursor position
 */
void display_set_cursor(int16_t x, int16_t y);

/**
 * @brief Set text size (1-4)
 */
void display_set_text_size(uint8_t size);

/**
 * @brief Set text color
 * @param fg Foreground color
 * @param bg Background color
 */
void display_set_text_color(uint16_t fg, uint16_t bg);

/**
 * @brief Print a string at current cursor position
 */
void display_print(const char *str);

/**
 * @brief Print a formatted string
 */
void display_printf(const char *fmt, ...);

/**
 * @brief Print a character
 */
void display_putc(char c);

// ============================================================================
//  Utility Functions
// ============================================================================

/**
 * @brief Convert RGB to RGB565 color
 * @param r Red (0-255)
 * @param g Green (0-255)
 * @param b Blue (0-255)
 * @return RGB565 color value
 */
uint16_t display_color565(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Get display width
 */
int16_t display_width(void);

/**
 * @brief Get display height
 */
int16_t display_height(void);

/**
 * @brief Get framebuffer pointer (if available)
 * @return Pointer to framebuffer, or NULL if not using double buffering
 */
uint16_t* display_get_framebuffer(void);

/**
 * @brief Flush framebuffer to display
 * Call after modifying framebuffer directly
 */
void display_flush(void);

/**
 * @brief Set display rotation
 * @param rotation 0-3 (0=Portrait, 1=Landscape, 2=Portrait flipped, 3=Landscape flipped)
 */
void display_set_rotation(uint8_t rotation);

#ifdef __cplusplus
}
#endif
