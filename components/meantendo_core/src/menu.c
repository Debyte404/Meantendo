/**
 * @file menu.c
 * @brief Meantendo Menu System - Implementation
 * @author Debyte
 * @version 2.0.0 (ESP-IDF Native)
 */

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "menu.h"
#include "meantendo_config.h"
#include "display.h"
#include "input.h"
#include "game_registry.h"

static const char *TAG = "MENU";

// ============================================================================
//  Static Variables
// ============================================================================

static int s_selected_game = 0;
static int s_scroll_offset = 0;
static bool s_in_game = false;

// ============================================================================
//  Helper Functions
// ============================================================================

static void ensure_selection_visible(bool wrapped) {
    int game_count = game_registry_count();
    
    if (game_count <= MENU_VISIBLE_ITEMS) {
        s_scroll_offset = 0;
        return;
    }
    
    if (wrapped) {
        if (s_selected_game == 0) {
            s_scroll_offset = 0;
        } else if (s_selected_game == game_count - 1) {
            s_scroll_offset = game_count - MENU_VISIBLE_ITEMS;
            if (s_scroll_offset < 0) s_scroll_offset = 0;
        }
        return;
    }
    
    if (s_selected_game < s_scroll_offset) {
        s_scroll_offset = s_selected_game;
    } else if (s_selected_game >= s_scroll_offset + MENU_VISIBLE_ITEMS) {
        s_scroll_offset = s_selected_game - MENU_VISIBLE_ITEMS + 1;
    }
}

// ============================================================================
//  Menu Drawing
// ============================================================================

void menu_draw_frame(void) {
    display_fill_screen(COLOR_BLACK);
    
    // Header
    const char *title = "Select Game";
    display_set_text_size(2);
    display_set_text_color(COLOR_YELLOW, COLOR_BLACK);
    int16_t x_header = (DISPLAY_WIDTH - strlen(title) * 12) / 2;
    display_set_cursor(x_header, 6);
    display_print(title);
    
    // Border
    display_draw_rect(2, 2, DISPLAY_WIDTH - 4, DISPLAY_HEIGHT - 4, COLOR_DARK_GREY);
    
    // Footer
    const char *footer = "w/ caffeine by Debyte";
    display_set_text_size(1);
    display_set_text_color(COLOR_DARK_GREY, COLOR_BLACK);
    int16_t x_footer = (DISPLAY_WIDTH - strlen(footer) * 6) / 2;
    display_set_cursor(x_footer, DISPLAY_HEIGHT - 15);
    display_print(footer);
}

void menu_draw_items(void) {
    // Clear menu area
    display_fill_rect(5, 25, DISPLAY_WIDTH - 10, DISPLAY_HEIGHT - 40, COLOR_BLACK);
    
    int game_count = game_registry_count();
    int visible = (MENU_VISIBLE_ITEMS < game_count) ? MENU_VISIBLE_ITEMS : game_count;
    
    display_set_text_size(2);
    
    for (int i = 0; i < visible; i++) {
        int game_index = i + s_scroll_offset;
        if (game_index >= game_count) break;
        
        const game_def_t *game = game_registry_get(game_index);
        if (!game) continue;
        
        int16_t y = MENU_START_Y + i * MENU_ITEM_HEIGHT;
        
        if (game_index == s_selected_game) {
            // Highlight bar
            display_fill_rect(10, y - 2, DISPLAY_WIDTH - 20, MENU_ITEM_HEIGHT + 1, COLOR_YELLOW);
            display_set_text_color(COLOR_BLACK, COLOR_YELLOW);
        } else {
            display_set_text_color(COLOR_WHITE, COLOR_BLACK);
        }
        
        int16_t x = (DISPLAY_WIDTH - strlen(game->name) * 12) / 2;
        display_set_cursor(x, y);
        display_print(game->name);
    }
}

// ============================================================================
//  Menu Logic
// ============================================================================

void menu_init(void) {
    int game_count = game_registry_count();
    
    if (s_selected_game >= game_count) {
        s_selected_game = (game_count > 0) ? game_count - 1 : 0;
    }
    
    if (game_count <= MENU_VISIBLE_ITEMS) {
        s_scroll_offset = 0;
    } else {
        ensure_selection_visible(false);
    }
    
    s_in_game = false;
    
    menu_draw_frame();
    menu_draw_items();
    
    ESP_LOGI(TAG, "Menu initialized with %d games", game_count);
}

void menu_move_selection(int delta) {
    int game_count = game_registry_count();
    if (game_count == 0) return;
    
    int prev = s_selected_game;
    s_selected_game = (s_selected_game + delta + game_count) % game_count;
    
    bool wrapped = (delta < 0 && prev == 0) || (delta > 0 && prev == game_count - 1);
    
    ensure_selection_visible(wrapped);
    menu_draw_items();
}

void menu_handle_input(void) {
    int game_count = game_registry_count();
    if (game_count == 0) return;
    
    // If in game, check for back button and run game loop
    if (s_in_game) {
        game_registry_tick();
        
        if (input_button_pressed(BTN_BACK)) {
            game_registry_stop_current();
            s_in_game = false;
            menu_draw_frame();
            menu_draw_items();
        }
        return;
    }
    
    // Menu navigation
    input_direction_t dir = input_read_joystick();
    
    if (dir == INPUT_DIR_UP) {
        menu_move_selection(-1);
        vTaskDelay(pdMS_TO_TICKS(150));  // Debounce
    } else if (dir == INPUT_DIR_DOWN) {
        menu_move_selection(1);
        vTaskDelay(pdMS_TO_TICKS(150));
    }
    
    // Select game
    if (input_button_pressed(BTN_SELECT) || input_button_pressed(BTN_A)) {
        display_fill_screen(COLOR_BLACK);
        game_registry_start(s_selected_game);
        s_in_game = true;
    }
}

int menu_get_selection(void) {
    return s_selected_game;
}

void menu_show(void) {
    menu_draw_frame();
    menu_draw_items();
}
