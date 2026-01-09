/**
 * @file pong.c
 * @brief Pong Game for Meantendo (Single & Multiplayer)
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

static const char *TAG = "PONG";

// ============================================================================
//  Game Constants
// ============================================================================

#define PADDLE_W        4
#define PADDLE_H        20
#define BALL_SIZE       4
#define SPEED_X         2
#define SPEED_Y         2

#define COLOR_BG        COLOR_BLACK
#define COLOR_PADDLE    COLOR_WHITE
#define COLOR_BALL      COLOR_YELLOW
#define COLOR_TEXT      COLOR_CYAN

// ============================================================================
//  Game State
// ============================================================================

typedef struct {
    int x, y;
} paddle_t;

typedef struct {
    int x, y, vx, vy;
} ball_t;

static paddle_t s_left_paddle, s_right_paddle;
static ball_t s_ball;
static int s_left_score, s_right_score;
static bool s_single_player;
static bool s_mode_selected;

// ============================================================================
//  Helper Functions
// ============================================================================

static void reset_ball(void) {
    s_ball.x = DISPLAY_WIDTH / 2 - BALL_SIZE / 2;
    s_ball.y = DISPLAY_HEIGHT / 2 - BALL_SIZE / 2;
    s_ball.vx = (esp_random() % 2 == 0) ? SPEED_X : -SPEED_X;
    s_ball.vy = (esp_random() % 2 == 0) ? SPEED_Y : -SPEED_Y;
}

static void draw_paddle(paddle_t p) {
    display_fill_rect(p.x, p.y, PADDLE_W, PADDLE_H, COLOR_PADDLE);
}

static void draw_ball(void) {
    display_fill_rect(s_ball.x, s_ball.y, BALL_SIZE, BALL_SIZE, COLOR_BALL);
}

static void draw_score(void) {
    display_set_text_size(1);
    display_set_text_color(COLOR_TEXT, COLOR_BG);
    display_set_cursor(10, 2);
    display_printf("%d", s_left_score);
    display_set_cursor(DISPLAY_WIDTH - 20, 2);
    display_printf("%d", s_right_score);
}

static void reset_game(void) {
    s_left_score = 0;
    s_right_score = 0;
    s_left_paddle.x = 5;
    s_left_paddle.y = DISPLAY_HEIGHT / 2 - PADDLE_H / 2;
    s_right_paddle.x = DISPLAY_WIDTH - PADDLE_W - 5;
    s_right_paddle.y = DISPLAY_HEIGHT / 2 - PADDLE_H / 2;
    reset_ball();
    display_fill_screen(COLOR_BG);
}

static void show_mode_select(void) {
    display_fill_screen(COLOR_BG);
    
    display_set_text_size(2);
    display_set_text_color(COLOR_YELLOW, COLOR_BG);
    display_set_cursor(50, 20);
    display_print("PONG");
    
    display_set_text_size(1);
    display_set_text_color(COLOR_WHITE, COLOR_BG);
    display_set_cursor(10, 60);
    display_print("Press A - 1 Player");
    display_set_cursor(10, 80);
    display_print("Press B - 2 Players");
    
    while (!s_mode_selected) {
        if (input_button_pressed(BTN_A)) {
            s_single_player = true;
            s_mode_selected = true;
        } else if (input_button_pressed(BTN_B)) {
            s_single_player = false;
            s_mode_selected = true;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    reset_game();
}

// ============================================================================
//  Game Callbacks
// ============================================================================

static void pong_start(void) {
    ESP_LOGI(TAG, "Starting Pong");
    s_mode_selected = false;
    show_mode_select();
}

static void pong_loop(void) {
    // Clear game area
    display_fill_rect(0, 10, DISPLAY_WIDTH, DISPLAY_HEIGHT - 10, COLOR_BG);
    
    // Player 1 paddle (joystick)
    input_direction_t dir = input_read_joystick();
    if (dir == INPUT_DIR_UP) s_left_paddle.y -= 3;
    else if (dir == INPUT_DIR_DOWN) s_left_paddle.y += 3;
    
    // Player 2 or AI
    if (s_single_player) {
        // Simple AI
        int ball_center = s_ball.y + BALL_SIZE / 2;
        int paddle_center = s_right_paddle.y + PADDLE_H / 2;
        if (ball_center > paddle_center + 2) s_right_paddle.y += 2;
        else if (ball_center < paddle_center - 2) s_right_paddle.y -= 2;
    } else {
        // Multiplayer: X/Y buttons
        if (input_button_pressed(BTN_X)) s_right_paddle.y -= 3;
        else if (input_button_pressed(BTN_Y)) s_right_paddle.y += 3;
    }
    
    // Constrain paddles
    if (s_left_paddle.y < 10) s_left_paddle.y = 10;
    if (s_left_paddle.y > DISPLAY_HEIGHT - PADDLE_H) s_left_paddle.y = DISPLAY_HEIGHT - PADDLE_H;
    if (s_right_paddle.y < 10) s_right_paddle.y = 10;
    if (s_right_paddle.y > DISPLAY_HEIGHT - PADDLE_H) s_right_paddle.y = DISPLAY_HEIGHT - PADDLE_H;
    
    // Move ball
    s_ball.x += s_ball.vx;
    s_ball.y += s_ball.vy;
    
    // Wall collision (top/bottom)
    if (s_ball.y <= 10 || s_ball.y >= DISPLAY_HEIGHT - BALL_SIZE) {
        s_ball.vy *= -1;
    }
    
    // Left paddle collision
    if (s_ball.x <= s_left_paddle.x + PADDLE_W &&
        s_ball.y + BALL_SIZE >= s_left_paddle.y &&
        s_ball.y <= s_left_paddle.y + PADDLE_H) {
        s_ball.vx = SPEED_X;  // Bounce right
    }
    
    // Right paddle collision
    if (s_ball.x + BALL_SIZE >= s_right_paddle.x &&
        s_ball.y + BALL_SIZE >= s_right_paddle.y &&
        s_ball.y <= s_right_paddle.y + PADDLE_H) {
        s_ball.vx = -SPEED_X;  // Bounce left
    }
    
    // Scoring
    if (s_ball.x <= 0) {
        s_right_score++;
        reset_ball();
    }
    if (s_ball.x >= DISPLAY_WIDTH - BALL_SIZE) {
        s_left_score++;
        reset_ball();
    }
    
    // Draw everything
    draw_paddle(s_left_paddle);
    draw_paddle(s_right_paddle);
    draw_ball();
    draw_score();
    
    // Draw center line
    for (int y = 10; y < DISPLAY_HEIGHT; y += 8) {
        display_fill_rect(DISPLAY_WIDTH / 2 - 1, y, 2, 4, COLOR_DARK_GREY);
    }
    
    vTaskDelay(pdMS_TO_TICKS(15));
}

static void pong_stop(void) {
    ESP_LOGI(TAG, "Pong stopped. Score: %d - %d", s_left_score, s_right_score);
}

// ============================================================================
//  Game Definition
// ============================================================================

const game_def_t game_pong = {
    .name = "Pong",
    .description = "Classic pong with AI or 2P",
    .icon = NULL,
    .start = pong_start,
    .loop = pong_loop,
    .stop = pong_stop,
    .requires_psram = false,
};
