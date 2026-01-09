/**
 * @file snake.c
 * @brief Classic Snake Game for Meantendo
 * @author Debyte
 * @version 2.0.0 (ESP-IDF Native)
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_random.h"

#include "meantendo_config.h"
#include "game_registry.h"
#include "display.h"
#include "input.h"

static const char *TAG = "SNAKE";

// ============================================================================
//  Game Constants
// ============================================================================

#define GRID_SIZE       8
#define GRID_W          (DISPLAY_WIDTH / GRID_SIZE)
#define GRID_H          (DISPLAY_HEIGHT / GRID_SIZE)
#define MAX_SNAKE_LEN   100

#define COLOR_BG        COLOR_BLACK
#define COLOR_SNAKE     COLOR_GREEN
#define COLOR_FOOD      COLOR_RED
#define COLOR_HEAD      0x07E0  // Bright green

// ============================================================================
//  Game State
// ============================================================================

typedef struct {
    int x;
    int y;
} point_t;

static point_t s_snake[MAX_SNAKE_LEN];
static int s_snake_length;
static point_t s_food;
static int s_dir_x, s_dir_y;
static int s_last_dir_x, s_last_dir_y;
static uint32_t s_last_move;
static int s_speed;
static bool s_game_over;
static int s_score;

// ============================================================================
//  Helper Functions
// ============================================================================

static inline bool collide(point_t a, point_t b) {
    return a.x == b.x && a.y == b.y;
}

static void draw_cell(point_t p, uint16_t color) {
    display_fill_rect(p.x * GRID_SIZE, p.y * GRID_SIZE, 
                      GRID_SIZE - 1, GRID_SIZE - 1, color);
}

static void place_food(void) {
    s_food.x = esp_random() % GRID_W;
    s_food.y = esp_random() % GRID_H;
}

static void reset_game(void) {
    s_snake_length = 3;
    s_dir_x = 1;
    s_dir_y = 0;
    s_last_dir_x = 1;
    s_last_dir_y = 0;
    s_speed = 150;
    s_game_over = false;
    s_score = 0;
    
    for (int i = 0; i < s_snake_length; i++) {
        s_snake[i].x = 3 - i;
        s_snake[i].y = 3;
    }
    
    place_food();
    display_fill_screen(COLOR_BG);
}

// ============================================================================
//  Game Callbacks
// ============================================================================

static void snake_start(void) {
    ESP_LOGI(TAG, "Starting Snake");
    reset_game();
    
    // Show title
    display_set_text_size(2);
    display_set_text_color(COLOR_YELLOW, COLOR_BG);
    display_set_cursor(40, 50);
    display_print("SNAKE!");
    vTaskDelay(pdMS_TO_TICKS(500));
    display_fill_screen(COLOR_BG);
    
    s_last_move = xTaskGetTickCount() * portTICK_PERIOD_MS;
}

static void snake_loop(void) {
    if (s_game_over) {
        display_set_text_size(2);
        display_set_text_color(COLOR_RED, COLOR_BG);
        display_set_cursor(15, 40);
        display_print("Game Over");
        
        display_set_text_size(1);
        display_set_text_color(COLOR_WHITE, COLOR_BG);
        display_set_cursor(35, 65);
        display_printf("Score: %d", s_score);
        display_set_cursor(20, 80);
        display_print("Press BACK");
        return;
    }
    
    // Read input
    input_direction_t dir = input_read_joystick_change();
    
    // Prevent 180° reverse
    if (dir == INPUT_DIR_UP && s_last_dir_y != 1) {
        s_dir_x = 0; s_dir_y = -1;
    } else if (dir == INPUT_DIR_DOWN && s_last_dir_y != -1) {
        s_dir_x = 0; s_dir_y = 1;
    } else if (dir == INPUT_DIR_LEFT && s_last_dir_x != 1) {
        s_dir_x = -1; s_dir_y = 0;
    } else if (dir == INPUT_DIR_RIGHT && s_last_dir_x != -1) {
        s_dir_x = 1; s_dir_y = 0;
    }
    
    // Movement timing
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    if (now - s_last_move < s_speed) return;
    s_last_move = now;
    
    // Calculate new head position
    point_t new_head = {
        .x = s_snake[0].x + s_dir_x,
        .y = s_snake[0].y + s_dir_y
    };
    
    // Wrap-around
    if (new_head.x < 0) new_head.x = GRID_W - 1;
    if (new_head.y < 0) new_head.y = GRID_H - 1;
    if (new_head.x >= GRID_W) new_head.x = 0;
    if (new_head.y >= GRID_H) new_head.y = 0;
    
    // Self-collision check
    for (int i = 0; i < s_snake_length; i++) {
        if (collide(new_head, s_snake[i])) {
            s_game_over = true;
            ESP_LOGI(TAG, "Game Over! Score: %d", s_score);
            return;
        }
    }
    
    // Move snake
    for (int i = s_snake_length - 1; i > 0; i--) {
        s_snake[i] = s_snake[i - 1];
    }
    s_snake[0] = new_head;
    
    // Check food collision
    if (collide(s_snake[0], s_food)) {
        if (s_snake_length < MAX_SNAKE_LEN - 1) {
            s_snake_length++;
            s_score += 10;
        }
        s_speed = (s_speed > 50) ? s_speed - 5 : 50;
        place_food();
    }
    
    // Draw frame
    display_fill_screen(COLOR_BG);
    draw_cell(s_food, COLOR_FOOD);
    
    // Draw snake (head is brighter)
    draw_cell(s_snake[0], COLOR_HEAD);
    for (int i = 1; i < s_snake_length; i++) {
        draw_cell(s_snake[i], COLOR_SNAKE);
    }
    
    // Store last direction
    s_last_dir_x = s_dir_x;
    s_last_dir_y = s_dir_y;
}

static void snake_stop(void) {
    ESP_LOGI(TAG, "Snake stopped, final score: %d", s_score);
}

// ============================================================================
//  Game Definition
// ============================================================================

const game_def_t game_snake = {
    .name = "Snake",
    .description = "Classic snake game",
    .icon = NULL,
    .start = snake_start,
    .loop = snake_loop,
    .stop = snake_stop,
    .requires_psram = false,
};
