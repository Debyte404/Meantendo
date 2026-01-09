/**
 * @file game_registry.c
 * @brief Meantendo Game Registry - Implementation
 * @author Debyte
 * @version 2.0.0 (ESP-IDF Native)
 */

#include <string.h>
#include "esp_log.h"
#include "game_registry.h"

static const char *TAG = "GAME_REG";

// ============================================================================
//  Static Variables
// ============================================================================

static const game_def_t *s_games[MAX_GAMES] = {NULL};
static int s_game_count = 0;
static int s_current_game = -1;
static bool s_game_running = false;

// ============================================================================
//  Implementation
// ============================================================================

void game_registry_init(void) {
    ESP_LOGI(TAG, "Initializing game registry...");
    
    s_game_count = 0;
    s_current_game = -1;
    s_game_running = false;
    
    // Clear registry
    memset(s_games, 0, sizeof(s_games));
    
    // Register built-in games
    game_registry_add(&game_snake);
    game_registry_add(&game_pong);
    
#if DOOM_SUPPORTED
    #if MEANTENDO_HAS_PSRAM
        ESP_LOGI(TAG, "PSRAM detected - DOOM is available!");
        game_registry_add(&game_doom);
    #else
        ESP_LOGW(TAG, "No PSRAM - DOOM will not be available");
    #endif
#endif
    
    ESP_LOGI(TAG, "Registered %d games", s_game_count);
}

int game_registry_add(const game_def_t *game) {
    if (game == NULL) {
        ESP_LOGE(TAG, "Cannot register NULL game");
        return -1;
    }
    
    if (s_game_count >= MAX_GAMES) {
        ESP_LOGE(TAG, "Game registry full (%d max)", MAX_GAMES);
        return -1;
    }
    
    // Check PSRAM requirement
    if (game->requires_psram && !MEANTENDO_HAS_PSRAM) {
        ESP_LOGW(TAG, "Skipping '%s' - requires PSRAM", game->name);
        return -1;
    }
    
    s_games[s_game_count] = game;
    ESP_LOGI(TAG, "Registered game: '%s'", game->name);
    
    return s_game_count++;
}

const game_def_t* game_registry_get(int index) {
    if (index < 0 || index >= s_game_count) {
        return NULL;
    }
    return s_games[index];
}

int game_registry_count(void) {
    return s_game_count;
}

int game_registry_find(const char *name) {
    if (name == NULL) return -1;
    
    for (int i = 0; i < s_game_count; i++) {
        if (s_games[i] && strcmp(s_games[i]->name, name) == 0) {
            return i;
        }
    }
    return -1;
}

bool game_registry_start(int index) {
    const game_def_t *game = game_registry_get(index);
    if (game == NULL) {
        ESP_LOGE(TAG, "Invalid game index: %d", index);
        return false;
    }
    
    // Stop current game if running
    if (s_game_running) {
        game_registry_stop_current();
    }
    
    ESP_LOGI(TAG, "Starting game: '%s'", game->name);
    
    s_current_game = index;
    s_game_running = true;
    
    // Call game start function
    if (game->start) {
        game->start();
    }
    
    return true;
}

void game_registry_stop_current(void) {
    if (!s_game_running || s_current_game < 0) {
        return;
    }
    
    const game_def_t *game = s_games[s_current_game];
    if (game && game->stop) {
        game->stop();
    }
    
    ESP_LOGI(TAG, "Stopped game: '%s'", game ? game->name : "unknown");
    
    s_current_game = -1;
    s_game_running = false;
}

int game_registry_current(void) {
    return s_current_game;
}

bool game_registry_is_running(void) {
    return s_game_running;
}

void game_registry_tick(void) {
    if (!s_game_running || s_current_game < 0) {
        return;
    }
    
    const game_def_t *game = s_games[s_current_game];
    if (game && game->loop) {
        game->loop();
    }
}
