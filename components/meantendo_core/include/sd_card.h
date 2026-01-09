/**
 * @file sd_card.h
 * @brief SD Card Helper for Meantendo
 */

#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Mount the SD card at the configured mount point (/sd)
 * @return ESP_OK on success
 */
esp_err_t sd_card_mount(void);

/**
 * @brief Unmount the SD card
 */
void sd_card_unmount(void);

/**
 * @brief Check if SD card is mounted
 */
bool sd_card_is_mounted(void);

#ifdef __cplusplus
}
#endif
