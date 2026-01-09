/**
 * @file seal_life.c
 * @brief Seal Life - Feed the hungry seal! 🦭
 * @author Debyte
 * @version 1.0.0
 * 
 * A cute seal-themed survival game where you guide a chubby grey seal
 * to eat fish continuously - or starve! Based on the snake concept but
 * with an adorable marine twist.
 * 
 * The seal is a fat grey blob (4x2 pixels) that gets hungrier over time.
 * Eat fish to survive and grow. Don't let the hunger bar empty!
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

static const char *TAG = "SEAL_LIFE";

// ============================================================================
//  Game Constants
// ============================================================================

#define SEAL_WIDTH      4
#define SEAL_HEIGHT     2
#define FISH_WIDTH      3
#define FISH_HEIGHT     2
#define MAX_FISH        5

// Colors 🦭
#define COLOR_BG        0x2945    // Deep ocean blue
#define COLOR_SEAL      0x8410    // Grey seal
#define COLOR_SEAL_FACE 0x0000    // Black eyes
#define COLOR_FISH      0xFD20    // Orange fish
#define COLOR_FISH_FIN  0xF800    // Red fin
#define COLOR_WATER_BG  0x1145    // Darker water
#define COLOR_HUNGER_OK 0x07E0    // Green
#define COLOR_HUNGER_LOW 0xFD20   // Orange
#define COLOR_HUNGER_CRIT 0xF800  // Red

// Hunger system
#define MAX_HUNGER      100
#define HUNGER_DRAIN    1         // Per tick
#define HUNGER_RESTORE  30        // Per fish eaten
#define TICK_INTERVAL   100       // ms

// ============================================================================
//  Game State
// ============================================================================

typedef struct {
    int x;
    int y;
    int vx;
    int vy;
    bool active;
} fish_t;

static int s_seal_x, s_seal_y;
static int s_seal_vx, s_seal_vy;
static int s_hunger;
static fish_t s_fish[MAX_FISH];
static int s_score;
static bool s_game_over;
static uint32_t s_last_tick;
static uint32_t s_last_move;
static int s_fish_eaten;

// ============================================================================
//  Drawing Functions
// ============================================================================

static void draw_seal(int x, int y) {
    // Main body - chubby grey blob (4x2)
    display_fill_rect(x, y, SEAL_WIDTH, SEAL_HEIGHT, COLOR_SEAL);
    
    // Little eye (1 pixel)
    int eye_x = (s_seal_vx < 0) ? x : x + SEAL_WIDTH - 1;
    display_draw_pixel(eye_x, y, COLOR_SEAL_FACE);
}

static void draw_fish(fish_t *f) {
    if (!f->active) return;
    
    // Fish body
    display_fill_rect(f->x, f->y, FISH_WIDTH, FISH_HEIGHT, COLOR_FISH);
    
    // Tail fin
    int tail_x = (f->vx > 0) ? f->x - 1 : f->x + FISH_WIDTH;
    if (tail_x >= 0 && tail_x < DISPLAY_WIDTH) {
        display_draw_pixel(tail_x, f->y, COLOR_FISH_FIN);
        display_draw_pixel(tail_x, f->y + 1, COLOR_FISH_FIN);
    }
}

static void draw_hunger_bar(void) {
    int bar_width = 60;
    int bar_height = 6;
    int x = DISPLAY_WIDTH - bar_width - 5;
    int y = 2;
    
    // Background
    display_fill_rect(x - 1, y - 1, bar_width + 2, bar_height + 2, COLOR_WATER_BG);
    
    // Fill based on hunger level
    int fill_width = (s_hunger * bar_width) / MAX_HUNGER;
    
    uint16_t bar_color;
    if (s_hunger > 60) {
        bar_color = COLOR_HUNGER_OK;
    } else if (s_hunger > 30) {
        bar_color = COLOR_HUNGER_LOW;
    } else {
        bar_color = COLOR_HUNGER_CRIT;
    }
    
    display_fill_rect(x, y, fill_width, bar_height, bar_color);
    
    // Label
    display_set_text_size(1);
    display_set_text_color(COLOR_WHITE, COLOR_BG);
    display_set_cursor(5, 2);
    display_print("HUNGER");
}

static void draw_score_display(void) {
    display_set_text_size(1);
    display_set_text_color(COLOR_WHITE, COLOR_BG);
    display_set_cursor(5, DISPLAY_HEIGHT - 10);
    display_printf("Fish: %d", s_fish_eaten);
}

static void draw_water_background(void) {
    display_fill_screen(COLOR_BG);
    
    // Add some subtle wave patterns
    for (int y = 15; y < DISPLAY_HEIGHT - 12; y += 20) {
        for (int x = 0; x < DISPLAY_WIDTH; x += 30) {
            int offset = (y / 20) % 2 ? 15 : 0;
            display_fill_rect(x + offset, y, 10, 1, COLOR_WATER_BG);
        }
    }
}

// ============================================================================
//  Game Logic
// ============================================================================

static void spawn_fish(void) {
    for (int i = 0; i < MAX_FISH; i++) {
        if (!s_fish[i].active) {
            // Spawn on random edge
            bool from_left = esp_random() % 2;
            s_fish[i].x = from_left ? -FISH_WIDTH : DISPLAY_WIDTH;
            s_fish[i].y = 15 + (esp_random() % (DISPLAY_HEIGHT - 30));
            s_fish[i].vx = from_left ? 1 : -1;
            s_fish[i].vy = 0;
            s_fish[i].active = true;
            break;
        }
    }
}

static void update_fish(void) {
    for (int i = 0; i < MAX_FISH; i++) {
        if (!s_fish[i].active) continue;
        
        // Move fish
        s_fish[i].x += s_fish[i].vx;
        
        // Random vertical movement occasionally
        if (esp_random() % 20 == 0) {
            s_fish[i].vy = ((esp_random() % 3) - 1);
        }
        s_fish[i].y += s_fish[i].vy;
        
        // Constrain Y
        if (s_fish[i].y < 12) s_fish[i].y = 12;
        if (s_fish[i].y > DISPLAY_HEIGHT - 15) s_fish[i].y = DISPLAY_HEIGHT - 15;
        
        // Remove if off screen
        if (s_fish[i].x < -FISH_WIDTH - 5 || s_fish[i].x > DISPLAY_WIDTH + 5) {
            s_fish[i].active = false;
        }
    }
    
    // Spawn new fish occasionally
    if (esp_random() % 30 == 0) {
        spawn_fish();
    }
}

static bool check_fish_collision(void) {
    for (int i = 0; i < MAX_FISH; i++) {
        if (!s_fish[i].active) continue;
        
        // AABB collision
        bool collision = 
            s_seal_x < s_fish[i].x + FISH_WIDTH &&
            s_seal_x + SEAL_WIDTH > s_fish[i].x &&
            s_seal_y < s_fish[i].y + FISH_HEIGHT &&
            s_seal_y + SEAL_HEIGHT > s_fish[i].y;
        
        if (collision) {
            s_fish[i].active = false;
            return true;
        }
    }
    return false;
}

static void reset_game(void) {
    s_seal_x = DISPLAY_WIDTH / 2 - SEAL_WIDTH / 2;
    s_seal_y = DISPLAY_HEIGHT / 2 - SEAL_HEIGHT / 2;
    s_seal_vx = 0;
    s_seal_vy = 0;
    s_hunger = MAX_HUNGER;
    s_score = 0;
    s_fish_eaten = 0;
    s_game_over = false;
    
    // Clear fish
    for (int i = 0; i < MAX_FISH; i++) {
        s_fish[i].active = false;
    }
    
    // Spawn initial fish
    for (int i = 0; i < 3; i++) {
        spawn_fish();
    }
    
    draw_water_background();
}

// ============================================================================
//  Game Callbacks
// ============================================================================

static void seal_life_start(void) {
    ESP_LOGI(TAG, "Starting Seal Life 🦭");
    reset_game();
    
    // Show title
    display_set_text_size(2);
    display_set_text_color(COLOR_WHITE, COLOR_BG);
    display_set_cursor(20, 40);
    display_print("SEAL LIFE");
    
    display_set_text_size(1);
    display_set_cursor(35, 70);
    display_print("Eat or Die!");
    
    vTaskDelay(pdMS_TO_TICKS(1500));
    draw_water_background();
    
    s_last_tick = xTaskGetTickCount() * portTICK_PERIOD_MS;
    s_last_move = s_last_tick;
}

static void seal_life_loop(void) {
    if (s_game_over) {
        display_set_text_size(2);
        display_set_text_color(COLOR_RED, COLOR_BG);
        display_set_cursor(20, 35);
        display_print("STARVED!");
        
        display_set_text_size(1);
        display_set_text_color(COLOR_WHITE, COLOR_BG);
        display_set_cursor(25, 60);
        display_printf("Fish eaten: %d", s_fish_eaten);
        display_set_cursor(25, 80);
        display_print("Press BACK");
        return;
    }
    
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    // Read input - seal swims in direction pushed
    input_direction_t dir = input_read_joystick();
    
    if (dir == INPUT_DIR_UP) {
        s_seal_vy = -2;
        s_seal_vx = 0;
    } else if (dir == INPUT_DIR_DOWN) {
        s_seal_vy = 2;
        s_seal_vx = 0;
    } else if (dir == INPUT_DIR_LEFT) {
        s_seal_vx = -2;
        s_seal_vy = 0;
    } else if (dir == INPUT_DIR_RIGHT) {
        s_seal_vx = 2;
        s_seal_vy = 0;
    } else {
        // Slowly decelerate
        if (s_seal_vx > 0) s_seal_vx--;
        else if (s_seal_vx < 0) s_seal_vx++;
        if (s_seal_vy > 0) s_seal_vy--;
        else if (s_seal_vy < 0) s_seal_vy++;
    }
    
    // Move seal
    if (now - s_last_move >= 30) {
        s_last_move = now;
        
        s_seal_x += s_seal_vx;
        s_seal_y += s_seal_vy;
        
        // Constrain to screen (seal can't escape!)
        if (s_seal_x < 0) s_seal_x = 0;
        if (s_seal_y < 12) s_seal_y = 12;
        if (s_seal_x > DISPLAY_WIDTH - SEAL_WIDTH) s_seal_x = DISPLAY_WIDTH - SEAL_WIDTH;
        if (s_seal_y > DISPLAY_HEIGHT - SEAL_HEIGHT - 10) s_seal_y = DISPLAY_HEIGHT - SEAL_HEIGHT - 10;
    }
    
    // Update hunger (every tick)
    if (now - s_last_tick >= TICK_INTERVAL) {
        s_last_tick = now;
        s_hunger -= HUNGER_DRAIN;
        
        if (s_hunger <= 0) {
            s_hunger = 0;
            s_game_over = true;
            ESP_LOGI(TAG, "Game Over! Fish eaten: %d", s_fish_eaten);
            return;
        }
        
        // Update fish
        update_fish();
    }
    
    // Check if seal ate a fish
    if (check_fish_collision()) {
        s_hunger += HUNGER_RESTORE;
        if (s_hunger > MAX_HUNGER) s_hunger = MAX_HUNGER;
        s_fish_eaten++;
        s_score += 100;
        ESP_LOGD(TAG, "Yum! Fish eaten: %d, Hunger: %d", s_fish_eaten, s_hunger);
    }
    
    // Draw everything
    draw_water_background();
    
    // Draw all fish
    for (int i = 0; i < MAX_FISH; i++) {
        draw_fish(&s_fish[i]);
    }
    
    // Draw seal
    draw_seal(s_seal_x, s_seal_y);
    
    // Draw UI
    draw_hunger_bar();
    draw_score_display();
}

static void seal_life_stop(void) {
    ESP_LOGI(TAG, "Seal Life ended. Fish eaten: %d", s_fish_eaten);
}

// ============================================================================
//  Game Definition
// ============================================================================

const game_def_t game_seal_life = {
    .name = "Seal Life",
    .description = "Feed the hungry seal!",
    .icon = NULL,
    .start = seal_life_start,
    .loop = seal_life_loop,
    .stop = seal_life_stop,
    .requires_psram = false,
};
