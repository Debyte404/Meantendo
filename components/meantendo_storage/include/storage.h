/**
 * @file storage.h
 * @brief Meantendo Storage Component - SD Card Management
 * @author Debyte
 * @version 1.0.0
 */

#pragma once

#include <esp_err.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the storage system (mount SD card)
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t storage_init(void);

/**
 * @brief Deinitialize the storage system (unmount SD card)
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t storage_deinit(void);

/**
 * @brief Check if SD card is currently mounted
 * 
 * @return true if mounted
 * @return false if not mounted
 */
bool storage_is_mounted(void);

/**
 * @brief Get total size of SD card in MB
 * 
 * @return uint32_t Size in MB, or 0 if not mounted
 */
uint32_t storage_get_total_space_mb(void);

/**
 * @brief Get free space on SD card in MB
 * 
 * @return uint32_t Free space in MB, or 0 if not mounted
 */
uint32_t storage_get_free_space_mb(void);

#ifdef __cplusplus
}
#endif
