/**
 * @file game_registry.h
 * @brief Meantendo Game Registry - Game Definition and Management
 * @author Debyte
 * @version 2.0.0 (ESP-IDF Native)
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "meantendo_config.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
//  Game Definition Structure
// ============================================================================

/**
 * @brief Game callback function type
 */
typedef void (*game_callback_t)(void);

/**
 * @brief Game definition structure
 */
typedef struct {
    const char *name;           ///< Display name
    const char *description;    ///< Short description
    const uint8_t *icon;        ///< 16x16 icon bitmap (optional)
    game_callback_t start;      ///< Called when game starts
    game_callback_t loop;       ///< Called each frame
    game_callback_t stop;       ///< Called when game exits (optional)
    bool requires_psram;        ///< True if game needs PSRAM (e.g., DOOM)
} game_def_t;

// ============================================================================
//  Game Registry API
// ============================================================================

/**
 * @brief Initialize the game registry and register built-in games
 */
void game_registry_init(void);

/**
 * @brief Register a new game
 * @param game Pointer to game definition
 * @return Index of registered game, or -1 on failure
 */
int game_registry_add(const game_def_t *game);

/**
 * @brief Get a game by index
 * @param index Game index
 * @return Pointer to game definition, or NULL if invalid
 */
const game_def_t* game_registry_get(int index);

/**
 * @brief Get total number of registered games
 * @return Game count
 */
int game_registry_count(void);

/**
 * @brief Find a game by name
 * @param name Game name to search
 * @return Game index, or -1 if not found
 */
int game_registry_find(const char *name);

/**
 * @brief Start a game by index
 * @param index Game index
 * @return true if game started successfully
 */
bool game_registry_start(int index);

/**
 * @brief Stop the currently running game
 */
void game_registry_stop_current(void);

/**
 * @brief Get the currently running game index
 * @return Current game index, or -1 if none
 */
int game_registry_current(void);

/**
 * @brief Check if a game is currently running
 * @return true if a game is active
 */
bool game_registry_is_running(void);

/**
 * @brief Execute one frame of the current game loop
 * Should be called from the main game task
 */
void game_registry_tick(void);

// ============================================================================
//  External Game Declarations (for registration)
// ============================================================================

// Built-in games
extern const game_def_t game_snake;
extern const game_def_t game_pong;

// Future games (DOOM will be here)
#if DOOM_SUPPORTED
extern const game_def_t game_doom;
#endif

#ifdef __cplusplus
}
#endif
