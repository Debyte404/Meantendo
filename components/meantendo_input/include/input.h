/**
 * @file input.h
 * @brief Meantendo Input System - Joystick and Buttons
 * @author Debyte
 * @version 2.0.0 (ESP-IDF Native)
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "meantendo_config.h"

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
//  Input Direction Enum
// ============================================================================

typedef enum {
    INPUT_DIR_NONE = 0,
    INPUT_DIR_UP,
    INPUT_DIR_DOWN,
    INPUT_DIR_LEFT,
    INPUT_DIR_RIGHT
} input_direction_t;

// ============================================================================
//  Button Identifiers
// ============================================================================

typedef enum {
    BTN_SELECT = 0,
    BTN_A,
    BTN_B,
    BTN_X,
    BTN_Y,
    BTN_BACK,
    BTN_COUNT
} input_button_t;

// ============================================================================
//  Input System API
// ============================================================================

/**
 * @brief Initialize the input system
 * @return ESP_OK on success
 */
esp_err_t input_init(void);

/**
 * @brief Deinitialize the input system
 */
void input_deinit(void);

// ============================================================================
//  Joystick Reading
// ============================================================================

/**
 * @brief Read joystick direction (continuous)
 * Returns the current direction while held
 * @return Current direction
 */
input_direction_t input_read_joystick(void);

/**
 * @brief Read joystick direction (state change)
 * Only returns a direction once when it changes
 * @return Direction if changed, INPUT_DIR_NONE otherwise
 */
input_direction_t input_read_joystick_change(void);

/**
 * @brief Get raw joystick X value (0-4095)
 */
int input_get_joystick_x(void);

/**
 * @brief Get raw joystick Y value (0-4095)
 */
int input_get_joystick_y(void);

// ============================================================================
//  Button Reading
// ============================================================================

/**
 * @brief Check if a button is currently pressed
 * @param button Button identifier
 * @return true if pressed
 */
bool input_button_pressed(input_button_t button);

/**
 * @brief Check if a button was just pressed (edge detection)
 * @param button Button identifier
 * @return true if just pressed this frame
 */
bool input_button_just_pressed(input_button_t button);

/**
 * @brief Check if a button was just released (edge detection)
 * @param button Button identifier
 * @return true if just released this frame
 */
bool input_button_just_released(input_button_t button);

/**
 * @brief Update input state (call once per frame)
 */
void input_update(void);

/**
 * @brief Check if any button is pressed
 */
bool input_any_button_pressed(void);

// ============================================================================
//  Debouncing
// ============================================================================

/**
 * @brief Get debounced button press (won't repeat for debounce period)
 * @param button Button identifier
 * @return true if pressed and debounce period passed
 */
bool input_button_debounced(input_button_t button);

#ifdef __cplusplus
}
#endif
