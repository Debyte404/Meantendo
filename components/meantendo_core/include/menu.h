/**
 * @file menu.h
 * @brief Meantendo Menu System
 * @author Debyte
 * @version 2.0.0 (ESP-IDF Native)
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the menu system
 */
void menu_init(void);

/**
 * @brief Draw the menu frame (header, footer, border)
 */
void menu_draw_frame(void);

/**
 * @brief Draw menu items with current selection
 */
void menu_draw_items(void);

/**
 * @brief Handle menu input and navigation
 * Should be called from the main loop
 */
void menu_handle_input(void);

/**
 * @brief Move selection up or down
 * @param delta -1 for up, +1 for down
 */
void menu_move_selection(int delta);

/**
 * @brief Get currently selected game index
 */
int menu_get_selection(void);

/**
 * @brief Show the menu (redraw completely)
 */
void menu_show(void);

#ifdef __cplusplus
}
#endif
