/**
 * @file main.c
 * @brief Meantendo Gaming Console - Main Application Entry Point
 * @author Debyte
 * @version 2.0.0 (ESP-IDF Native)
 * 
 * Native ESP-IDF implementation of the Meantendo gaming console.
 * Supports PSRAM for advanced games like DOOM.
 */

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "nvs_flash.h"

// Meantendo Components
#include "meantendo_config.h"
#include "display.h"
#include "input.h"
#include "menu.h"
#include "splash.h"
#include "game_registry.h"

static const char *TAG = "MEANTENDO";

// ============================================================================
//  System Initialization
// ============================================================================

/**
 * @brief Initialize NVS (Non-Volatile Storage)
 */
static esp_err_t init_nvs(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    return ret;
}

/**
 * @brief Print system information
 */
static void print_system_info(void) {
    ESP_LOGI(TAG, "═══════════════════════════════════════════════════════════════");
    ESP_LOGI(TAG, "  MEANTENDO Gaming Console v%s", MEANTENDO_VERSION_STRING);
    ESP_LOGI(TAG, "  Built: %s %s", __DATE__, __TIME__);
    ESP_LOGI(TAG, "═══════════════════════════════════════════════════════════════");
    
    // Memory info
    ESP_LOGI(TAG, "  Free heap: %lu bytes", (unsigned long)esp_get_free_heap_size());
    ESP_LOGI(TAG, "  Minimum free heap: %lu bytes", (unsigned long)esp_get_minimum_free_heap_size());
    
#ifdef CONFIG_SPIRAM
    size_t psram_size = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
    size_t psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    ESP_LOGI(TAG, "  PSRAM: %lu / %lu bytes free", (unsigned long)psram_free, (unsigned long)psram_size);
#else
    ESP_LOGW(TAG, "  PSRAM: Not available (DOOM may not work)");
#endif
    
    ESP_LOGI(TAG, "═══════════════════════════════════════════════════════════════");
}

// ============================================================================
//  Game Loop Task
// ============================================================================

static void game_loop_task(void *pvParameters) {
    ESP_LOGI(TAG, "Starting game loop...");
    
    while (1) {
        // Handle menu input and game execution
        menu_handle_input();
        
        // Yield to other tasks
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ============================================================================
//  Main Application Entry
// ============================================================================

void app_main(void) {
    ESP_LOGI(TAG, "Initializing Meantendo Gaming Console...");
    
    // Initialize NVS
    ESP_ERROR_CHECK(init_nvs());
    
    // Print system information
    print_system_info();
    
    // Initialize display
    ESP_LOGI(TAG, "Initializing display...");
    ESP_ERROR_CHECK(display_init());
    
    // Initialize input system
    ESP_LOGI(TAG, "Initializing input system...");
    ESP_ERROR_CHECK(input_init());
    
    // Register built-in games
    ESP_LOGI(TAG, "Registering games...");
    game_registry_init();
    
    // Show splash screen
    ESP_LOGI(TAG, "Showing splash screen...");
    splash_show();
    
    // Initialize menu
    ESP_LOGI(TAG, "Initializing menu...");
    menu_init();
    
    // Create game loop task
    xTaskCreatePinnedToCore(
        game_loop_task,
        "game_loop",
        8192,           // Stack size (increased for games)
        NULL,
        5,              // Priority
        NULL,
        1               // Core 1 (keep networking on Core 0)
    );
    
    ESP_LOGI(TAG, "Meantendo is ready! Enjoy gaming!");
}
