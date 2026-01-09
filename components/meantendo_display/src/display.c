/**
 * @file display.c
 * @brief Meantendo Display Driver - ST7735 TFT Implementation
 * @author Debyte
 * @version 2.0.0 (ESP-IDF Native)
 * 
 * High-performance display driver using ESP-IDF SPI DMA
 */

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

#include "display.h"
#include "meantendo_config.h"

static const char *TAG = "DISPLAY";

// ============================================================================
//  ST7735 Commands
// ============================================================================

#define ST7735_NOP      0x00
#define ST7735_SWRESET  0x01
#define ST7735_SLPOUT   0x11
#define ST7735_NORON    0x13
#define ST7735_INVOFF   0x20
#define ST7735_INVON    0x21
#define ST7735_DISPOFF  0x28
#define ST7735_DISPON   0x29
#define ST7735_CASET    0x2A
#define ST7735_RASET    0x2B
#define ST7735_RAMWR    0x2C
#define ST7735_MADCTL   0x36
#define ST7735_COLMOD   0x3A
#define ST7735_FRMCTR1  0xB1
#define ST7735_FRMCTR2  0xB2
#define ST7735_FRMCTR3  0xB3
#define ST7735_INVCTR   0xB4
#define ST7735_PWCTR1   0xC0
#define ST7735_PWCTR2   0xC1
#define ST7735_PWCTR3   0xC2
#define ST7735_PWCTR4   0xC3
#define ST7735_PWCTR5   0xC4
#define ST7735_VMCTR1   0xC5
#define ST7735_GMCTRP1  0xE0
#define ST7735_GMCTRN1  0xE1

// MADCTL bits
#define MADCTL_MY   0x80
#define MADCTL_MX   0x40
#define MADCTL_MV   0x20
#define MADCTL_ML   0x10
#define MADCTL_RGB  0x00
#define MADCTL_BGR  0x08

// ============================================================================
//  Static Variables
// ============================================================================

static spi_device_handle_t s_spi = NULL;
static uint16_t *s_framebuffer = NULL;
static bool s_initialized = false;

// Text state
static int16_t s_cursor_x = 0;
static int16_t s_cursor_y = 0;
static uint8_t s_text_size = 1;
static uint16_t s_text_fg = COLOR_WHITE;
static uint16_t s_text_bg = COLOR_BLACK;

// Column/row offsets (for ST7735 variants)
static uint8_t s_col_offset = 0;
static uint8_t s_row_offset = 0;

// ============================================================================
//  Basic Font (5x7)
// ============================================================================

static const uint8_t font5x7[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, // (space)
    0x00, 0x00, 0x5F, 0x00, 0x00, // !
    0x00, 0x07, 0x00, 0x07, 0x00, // "
    0x14, 0x7F, 0x14, 0x7F, 0x14, // #
    0x24, 0x2A, 0x7F, 0x2A, 0x12, // $
    0x23, 0x13, 0x08, 0x64, 0x62, // %
    0x36, 0x49, 0x55, 0x22, 0x50, // &
    0x00, 0x05, 0x03, 0x00, 0x00, // '
    0x00, 0x1C, 0x22, 0x41, 0x00, // (
    0x00, 0x41, 0x22, 0x1C, 0x00, // )
    0x08, 0x2A, 0x1C, 0x2A, 0x08, // *
    0x08, 0x08, 0x3E, 0x08, 0x08, // +
    0x00, 0x50, 0x30, 0x00, 0x00, // ,
    0x08, 0x08, 0x08, 0x08, 0x08, // -
    0x00, 0x60, 0x60, 0x00, 0x00, // .
    0x20, 0x10, 0x08, 0x04, 0x02, // /
    0x3E, 0x51, 0x49, 0x45, 0x3E, // 0
    0x00, 0x42, 0x7F, 0x40, 0x00, // 1
    0x42, 0x61, 0x51, 0x49, 0x46, // 2
    0x21, 0x41, 0x45, 0x4B, 0x31, // 3
    0x18, 0x14, 0x12, 0x7F, 0x10, // 4
    0x27, 0x45, 0x45, 0x45, 0x39, // 5
    0x3C, 0x4A, 0x49, 0x49, 0x30, // 6
    0x01, 0x71, 0x09, 0x05, 0x03, // 7
    0x36, 0x49, 0x49, 0x49, 0x36, // 8
    0x06, 0x49, 0x49, 0x29, 0x1E, // 9
    0x00, 0x36, 0x36, 0x00, 0x00, // :
    0x00, 0x56, 0x36, 0x00, 0x00, // ;
    0x00, 0x08, 0x14, 0x22, 0x41, // <
    0x14, 0x14, 0x14, 0x14, 0x14, // =
    0x41, 0x22, 0x14, 0x08, 0x00, // >
    0x02, 0x01, 0x51, 0x09, 0x06, // ?
    0x32, 0x49, 0x79, 0x41, 0x3E, // @
    0x7E, 0x11, 0x11, 0x11, 0x7E, // A
    0x7F, 0x49, 0x49, 0x49, 0x36, // B
    0x3E, 0x41, 0x41, 0x41, 0x22, // C
    0x7F, 0x41, 0x41, 0x22, 0x1C, // D
    0x7F, 0x49, 0x49, 0x49, 0x41, // E
    0x7F, 0x09, 0x09, 0x01, 0x01, // F
    0x3E, 0x41, 0x41, 0x51, 0x32, // G
    0x7F, 0x08, 0x08, 0x08, 0x7F, // H
    0x00, 0x41, 0x7F, 0x41, 0x00, // I
    0x20, 0x40, 0x41, 0x3F, 0x01, // J
    0x7F, 0x08, 0x14, 0x22, 0x41, // K
    0x7F, 0x40, 0x40, 0x40, 0x40, // L
    0x7F, 0x02, 0x04, 0x02, 0x7F, // M
    0x7F, 0x04, 0x08, 0x10, 0x7F, // N
    0x3E, 0x41, 0x41, 0x41, 0x3E, // O
    0x7F, 0x09, 0x09, 0x09, 0x06, // P
    0x3E, 0x41, 0x51, 0x21, 0x5E, // Q
    0x7F, 0x09, 0x19, 0x29, 0x46, // R
    0x46, 0x49, 0x49, 0x49, 0x31, // S
    0x01, 0x01, 0x7F, 0x01, 0x01, // T
    0x3F, 0x40, 0x40, 0x40, 0x3F, // U
    0x1F, 0x20, 0x40, 0x20, 0x1F, // V
    0x7F, 0x20, 0x18, 0x20, 0x7F, // W
    0x63, 0x14, 0x08, 0x14, 0x63, // X
    0x03, 0x04, 0x78, 0x04, 0x03, // Y
    0x61, 0x51, 0x49, 0x45, 0x43, // Z
    0x00, 0x00, 0x7F, 0x41, 0x41, // [
    0x02, 0x04, 0x08, 0x10, 0x20, // backslash
    0x41, 0x41, 0x7F, 0x00, 0x00, // ]
    0x04, 0x02, 0x01, 0x02, 0x04, // ^
    0x40, 0x40, 0x40, 0x40, 0x40, // _
    0x00, 0x01, 0x02, 0x04, 0x00, // `
    0x20, 0x54, 0x54, 0x54, 0x78, // a
    0x7F, 0x48, 0x44, 0x44, 0x38, // b
    0x38, 0x44, 0x44, 0x44, 0x20, // c
    0x38, 0x44, 0x44, 0x48, 0x7F, // d
    0x38, 0x54, 0x54, 0x54, 0x18, // e
    0x08, 0x7E, 0x09, 0x01, 0x02, // f
    0x08, 0x14, 0x54, 0x54, 0x3C, // g
    0x7F, 0x08, 0x04, 0x04, 0x78, // h
    0x00, 0x44, 0x7D, 0x40, 0x00, // i
    0x20, 0x40, 0x44, 0x3D, 0x00, // j
    0x00, 0x7F, 0x10, 0x28, 0x44, // k
    0x00, 0x41, 0x7F, 0x40, 0x00, // l
    0x7C, 0x04, 0x18, 0x04, 0x78, // m
    0x7C, 0x08, 0x04, 0x04, 0x78, // n
    0x38, 0x44, 0x44, 0x44, 0x38, // o
    0x7C, 0x14, 0x14, 0x14, 0x08, // p
    0x08, 0x14, 0x14, 0x18, 0x7C, // q
    0x7C, 0x08, 0x04, 0x04, 0x08, // r
    0x48, 0x54, 0x54, 0x54, 0x20, // s
    0x04, 0x3F, 0x44, 0x40, 0x20, // t
    0x3C, 0x40, 0x40, 0x20, 0x7C, // u
    0x1C, 0x20, 0x40, 0x20, 0x1C, // v
    0x3C, 0x40, 0x30, 0x40, 0x3C, // w
    0x44, 0x28, 0x10, 0x28, 0x44, // x
    0x0C, 0x50, 0x50, 0x50, 0x3C, // y
    0x44, 0x64, 0x54, 0x4C, 0x44, // z
    0x00, 0x08, 0x36, 0x41, 0x00, // {
    0x00, 0x00, 0x7F, 0x00, 0x00, // |
    0x00, 0x41, 0x36, 0x08, 0x00, // }
    0x08, 0x08, 0x2A, 0x1C, 0x08, // ->
    0x08, 0x1C, 0x2A, 0x08, 0x08, // <-
};

// ============================================================================
//  SPI Helper Functions
// ============================================================================

static void spi_send_cmd(uint8_t cmd) {
    gpio_set_level(DISPLAY_PIN_DC, 0);
    spi_transaction_t t = {
        .length = 8,
        .tx_buffer = &cmd,
    };
    spi_device_transmit(s_spi, &t);
}

static void spi_send_data(const uint8_t *data, size_t len) {
    if (len == 0) return;
    gpio_set_level(DISPLAY_PIN_DC, 1);
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
    };
    spi_device_transmit(s_spi, &t);
}

static void spi_send_data_byte(uint8_t data) {
    spi_send_data(&data, 1);
}

static void set_addr_window(int16_t x, int16_t y, int16_t w, int16_t h) {
    uint8_t data[4];
    
    // Column address
    spi_send_cmd(ST7735_CASET);
    data[0] = 0;
    data[1] = x + s_col_offset;
    data[2] = 0;
    data[3] = x + w - 1 + s_col_offset;
    spi_send_data(data, 4);
    
    // Row address
    spi_send_cmd(ST7735_RASET);
    data[0] = 0;
    data[1] = y + s_row_offset;
    data[2] = 0;
    data[3] = y + h - 1 + s_row_offset;
    spi_send_data(data, 4);
    
    // Write to RAM
    spi_send_cmd(ST7735_RAMWR);
}

// ============================================================================
//  Display Initialization
// ============================================================================

esp_err_t display_init(void) {
    if (s_initialized) {
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Initializing ST7735 display...");
    
    // Configure GPIO pins
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << DISPLAY_PIN_DC) | (1ULL << DISPLAY_PIN_RST),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    
    // Configure SPI bus
    spi_bus_config_t buscfg = {
        .mosi_io_num = DISPLAY_PIN_MOSI,
        .miso_io_num = -1,
        .sclk_io_num = DISPLAY_PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * 2 + 8,
    };
    
    ESP_ERROR_CHECK(spi_bus_initialize(DISPLAY_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO));
    
    // Configure SPI device
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = DISPLAY_SPI_FREQ_HZ,
        .mode = 0,
        .spics_io_num = DISPLAY_PIN_CS,
        .queue_size = DISPLAY_QUEUE_SIZE,
        .pre_cb = NULL,
    };
    
    ESP_ERROR_CHECK(spi_bus_add_device(DISPLAY_SPI_HOST, &devcfg, &s_spi));
    
    // Hardware reset
    gpio_set_level(DISPLAY_PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(DISPLAY_PIN_RST, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(DISPLAY_PIN_RST, 1);
    vTaskDelay(pdMS_TO_TICKS(120));
    
    // Initialize display
    spi_send_cmd(ST7735_SWRESET);
    vTaskDelay(pdMS_TO_TICKS(150));
    
    spi_send_cmd(ST7735_SLPOUT);
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Frame rate control
    spi_send_cmd(ST7735_FRMCTR1);
    uint8_t frmctr[] = {0x01, 0x2C, 0x2D};
    spi_send_data(frmctr, 3);
    
    spi_send_cmd(ST7735_FRMCTR2);
    spi_send_data(frmctr, 3);
    
    spi_send_cmd(ST7735_FRMCTR3);
    spi_send_data(frmctr, 3);
    spi_send_data(frmctr, 3);
    
    // Power control
    spi_send_cmd(ST7735_INVCTR);
    spi_send_data_byte(0x07);
    
    spi_send_cmd(ST7735_PWCTR1);
    uint8_t pwctr1[] = {0xA2, 0x02, 0x84};
    spi_send_data(pwctr1, 3);
    
    spi_send_cmd(ST7735_PWCTR2);
    spi_send_data_byte(0xC5);
    
    spi_send_cmd(ST7735_PWCTR3);
    uint8_t pwctr3[] = {0x0A, 0x00};
    spi_send_data(pwctr3, 2);
    
    spi_send_cmd(ST7735_PWCTR4);
    uint8_t pwctr4[] = {0x8A, 0x2A};
    spi_send_data(pwctr4, 2);
    
    spi_send_cmd(ST7735_PWCTR5);
    uint8_t pwctr5[] = {0x8A, 0xEE};
    spi_send_data(pwctr5, 2);
    
    spi_send_cmd(ST7735_VMCTR1);
    spi_send_data_byte(0x0E);
    
    spi_send_cmd(ST7735_INVOFF);
    
    // Set pixel format to 16-bit RGB565
    spi_send_cmd(ST7735_COLMOD);
    spi_send_data_byte(0x05);
    
    // Set rotation to landscape
    display_set_rotation(DISPLAY_ROTATION);
    
    // Normal display mode
    spi_send_cmd(ST7735_NORON);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    spi_send_cmd(ST7735_DISPON);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Allocate framebuffer in PSRAM if available
#ifdef CONFIG_SPIRAM
    s_framebuffer = heap_caps_malloc(FRAMEBUFFER_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (s_framebuffer) {
        ESP_LOGI(TAG, "Framebuffer allocated in PSRAM");
    }
#endif
    if (!s_framebuffer) {
        s_framebuffer = heap_caps_malloc(FRAMEBUFFER_SIZE, MALLOC_CAP_DMA);
        if (s_framebuffer) {
            ESP_LOGI(TAG, "Framebuffer allocated in internal RAM");
        }
    }
    
    // Clear screen
    display_fill_screen(COLOR_BLACK);
    
    s_initialized = true;
    ESP_LOGI(TAG, "Display initialized: %dx%d", DISPLAY_WIDTH, DISPLAY_HEIGHT);
    
    return ESP_OK;
}

void display_deinit(void) {
    if (s_framebuffer) {
        free(s_framebuffer);
        s_framebuffer = NULL;
    }
    if (s_spi) {
        spi_bus_remove_device(s_spi);
        spi_bus_free(DISPLAY_SPI_HOST);
        s_spi = NULL;
    }
    s_initialized = false;
}

// ============================================================================
//  Drawing Functions
// ============================================================================

void display_set_rotation(uint8_t rotation) {
    uint8_t madctl = 0;
    
    switch (rotation % 4) {
        case 0:  // Portrait
            madctl = MADCTL_MX | MADCTL_MY | MADCTL_BGR;
            s_col_offset = 0;
            s_row_offset = 0;
            break;
        case 1:  // Landscape
            madctl = MADCTL_MY | MADCTL_MV | MADCTL_BGR;
            s_col_offset = 0;
            s_row_offset = 0;
            break;
        case 2:  // Portrait inverted
            madctl = MADCTL_BGR;
            s_col_offset = 0;
            s_row_offset = 0;
            break;
        case 3:  // Landscape inverted
            madctl = MADCTL_MX | MADCTL_MV | MADCTL_BGR;
            s_col_offset = 0;
            s_row_offset = 0;
            break;
    }
    
    spi_send_cmd(ST7735_MADCTL);
    spi_send_data_byte(madctl);
}

void display_fill_screen(uint16_t color) {
    display_fill_rect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, color);
}

void display_draw_pixel(int16_t x, int16_t y, uint16_t color) {
    if (x < 0 || x >= DISPLAY_WIDTH || y < 0 || y >= DISPLAY_HEIGHT) return;
    
    set_addr_window(x, y, 1, 1);
    gpio_set_level(DISPLAY_PIN_DC, 1);
    
    uint8_t data[2] = {color >> 8, color & 0xFF};
    spi_send_data(data, 2);
}

void display_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT || w <= 0 || h <= 0) return;
    
    // Clipping
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > DISPLAY_WIDTH) w = DISPLAY_WIDTH - x;
    if (y + h > DISPLAY_HEIGHT) h = DISPLAY_HEIGHT - y;
    
    set_addr_window(x, y, w, h);
    gpio_set_level(DISPLAY_PIN_DC, 1);
    
    // Use a line buffer for efficiency
    size_t pixels = w * h;
    uint16_t swapped = (color >> 8) | (color << 8);  // Swap bytes for SPI
    
    // Use DMA-capable buffer
    size_t buf_pixels = (w < 160) ? w : 160;
    uint16_t *buf = heap_caps_malloc(buf_pixels * 2, MALLOC_CAP_DMA);
    if (!buf) {
        ESP_LOGE(TAG, "Failed to allocate line buffer");
        return;
    }
    
    for (size_t i = 0; i < buf_pixels; i++) {
        buf[i] = swapped;
    }
    
    spi_transaction_t t = {
        .tx_buffer = buf,
    };
    
    size_t remaining = pixels;
    while (remaining > 0) {
        size_t chunk = (remaining > buf_pixels) ? buf_pixels : remaining;
        t.length = chunk * 16;
        spi_device_transmit(s_spi, &t);
        remaining -= chunk;
    }
    
    free(buf);
}

void display_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    display_draw_hline(x, y, w, color);
    display_draw_hline(x, y + h - 1, w, color);
    display_draw_vline(x, y, h, color);
    display_draw_vline(x + w - 1, y, h, color);
}

void display_draw_hline(int16_t x, int16_t y, int16_t w, uint16_t color) {
    display_fill_rect(x, y, w, 1, color);
}

void display_draw_vline(int16_t x, int16_t y, int16_t h, uint16_t color) {
    display_fill_rect(x, y, 1, h, color);
}

void display_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    int16_t steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) {
        int16_t t = x0; x0 = y0; y0 = t;
        t = x1; x1 = y1; y1 = t;
    }
    if (x0 > x1) {
        int16_t t = x0; x0 = x1; x1 = t;
        t = y0; y0 = y1; y1 = t;
    }
    
    int16_t dx = x1 - x0;
    int16_t dy = abs(y1 - y0);
    int16_t err = dx / 2;
    int16_t ystep = (y0 < y1) ? 1 : -1;
    
    for (; x0 <= x1; x0++) {
        if (steep) {
            display_draw_pixel(y0, x0, color);
        } else {
            display_draw_pixel(x0, y0, color);
        }
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

void display_draw_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    
    display_draw_pixel(x0, y0 + r, color);
    display_draw_pixel(x0, y0 - r, color);
    display_draw_pixel(x0 + r, y0, color);
    display_draw_pixel(x0 - r, y0, color);
    
    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        
        display_draw_pixel(x0 + x, y0 + y, color);
        display_draw_pixel(x0 - x, y0 + y, color);
        display_draw_pixel(x0 + x, y0 - y, color);
        display_draw_pixel(x0 - x, y0 - y, color);
        display_draw_pixel(x0 + y, y0 + x, color);
        display_draw_pixel(x0 - y, y0 + x, color);
        display_draw_pixel(x0 + y, y0 - x, color);
        display_draw_pixel(x0 - y, y0 - x, color);
    }
}

void display_fill_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color) {
    display_draw_vline(x0, y0 - r, 2 * r + 1, color);
    
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;
    
    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;
        
        display_draw_vline(x0 + x, y0 - y, 2 * y + 1, color);
        display_draw_vline(x0 - x, y0 - y, 2 * y + 1, color);
        display_draw_vline(x0 + y, y0 - x, 2 * x + 1, color);
        display_draw_vline(x0 - y, y0 - x, 2 * x + 1, color);
    }
}

// ============================================================================
//  Bitmap Drawing
// ============================================================================

void display_draw_bitmap_vertical(int16_t x, int16_t y, const uint8_t *bitmap,
                                   int16_t w, int16_t h, uint16_t color) {
    for (int16_t j = 0; j < h; j++) {
        for (int16_t i = 0; i < w; i++) {
            uint8_t byte = bitmap[i + (j / 8) * w];
            if (byte & (1 << (j & 7))) {
                display_draw_pixel(x + i, y + j, color);
            }
        }
    }
}

void display_draw_rgb565(int16_t x, int16_t y, const uint16_t *bitmap, int16_t w, int16_t h) {
    display_blit(x, y, w, h, bitmap);
}

void display_blit(int16_t x, int16_t y, int16_t w, int16_t h, const uint16_t *data) {
    if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT || !data) return;
    
    set_addr_window(x, y, w, h);
    gpio_set_level(DISPLAY_PIN_DC, 1);
    
    // Swap bytes for SPI transmission
    size_t pixels = w * h;
    
    // Use DMA buffer
    uint16_t *buf = heap_caps_malloc(pixels * 2, MALLOC_CAP_DMA);
    if (buf) {
        for (size_t i = 0; i < pixels; i++) {
            buf[i] = (data[i] >> 8) | (data[i] << 8);
        }
        
        spi_transaction_t t = {
            .length = pixels * 16,
            .tx_buffer = buf,
        };
        spi_device_transmit(s_spi, &t);
        free(buf);
    } else {
        // Fallback: send pixel by pixel
        for (size_t i = 0; i < pixels; i++) {
            uint8_t d[2] = {data[i] >> 8, data[i] & 0xFF};
            spi_send_data(d, 2);
        }
    }
}

// ============================================================================
//  Text Drawing
// ============================================================================

void display_set_cursor(int16_t x, int16_t y) {
    s_cursor_x = x;
    s_cursor_y = y;
}

void display_set_text_size(uint8_t size) {
    s_text_size = (size > 0) ? size : 1;
}

void display_set_text_color(uint16_t fg, uint16_t bg) {
    s_text_fg = fg;
    s_text_bg = bg;
}

void display_putc(char c) {
    if (c == '\n') {
        s_cursor_y += s_text_size * 8;
        s_cursor_x = 0;
        return;
    }
    if (c == '\r') {
        s_cursor_x = 0;
        return;
    }
    
    if (c < 32 || c > 126) c = '?';
    
    const uint8_t *glyph = &font5x7[(c - 32) * 5];
    
    for (int8_t i = 0; i < 5; i++) {
        uint8_t line = glyph[i];
        for (int8_t j = 0; j < 7; j++) {
            uint16_t color = (line & (1 << j)) ? s_text_fg : s_text_bg;
            if (s_text_size == 1) {
                display_draw_pixel(s_cursor_x + i, s_cursor_y + j, color);
            } else {
                display_fill_rect(s_cursor_x + i * s_text_size,
                                 s_cursor_y + j * s_text_size,
                                 s_text_size, s_text_size, color);
            }
        }
    }
    
    // Draw background for spacing column
    for (int8_t j = 0; j < 7; j++) {
        if (s_text_size == 1) {
            display_draw_pixel(s_cursor_x + 5, s_cursor_y + j, s_text_bg);
        } else {
            display_fill_rect(s_cursor_x + 5 * s_text_size,
                             s_cursor_y + j * s_text_size,
                             s_text_size, s_text_size, s_text_bg);
        }
    }
    
    s_cursor_x += s_text_size * 6;
}

void display_print(const char *str) {
    while (*str) {
        display_putc(*str++);
    }
}

void display_printf(const char *fmt, ...) {
    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    display_print(buf);
}

// ============================================================================
//  Utility Functions
// ============================================================================

uint16_t display_color565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

int16_t display_width(void) {
    return DISPLAY_WIDTH;
}

int16_t display_height(void) {
    return DISPLAY_HEIGHT;
}

uint16_t* display_get_framebuffer(void) {
    return s_framebuffer;
}

void display_flush(void) {
    if (s_framebuffer) {
        display_blit(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, s_framebuffer);
    }
}
