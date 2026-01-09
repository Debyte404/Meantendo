/**
 * @file doom_game.c
 * @brief DOOM Integration for Meantendo
 * @author Debyte
 * @version 2.0.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_partition.h"
#include "esp_vfs_fat.h"
#include "esp_spiffs.h"

#include "meantendo_config.h"
#include "game_registry.h"
#include "doom_app.h"
#include "display.h"
#include "storage.h"
#include <unistd.h>

static const char *TAG = "DOOM_GAME";

// ============================================================================
//  WAD Management
// ============================================================================

static bool mount_wad_partition(void) {
    // We treating the WAD partition as a raw data blob usually, 
    // but DOOM expects a file path.
    // Ideally we mount it as a read-only filesystem (like SPIFFS or FATFS).
    // Or we modify DOOM to read from memory mapped flash.
    
    // For simplicity with standard DOOM code, we'll try to use SPIFFS
    // mapped to the 'wad' partition label we defined in partitions.csv
    
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/wad",
      .partition_label = "wad",
      .max_files = 5,
      .format_if_mount_failed = false
    };
    
    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return false;
    }
    
    ESP_LOGI(TAG, "WAD Partition mounted at /wad");
    return true;
}

// ============================================================================
//  Game Loop Task
// ============================================================================

static void doom_task(void *arg) {
    ESP_LOGI(TAG, "DOOM Task Started");
    
    // Arguments for DOOM
    // We point to the WAD file we assume is in the partition
    // If SD card is mounted, we tell DOOM to use it for saves
    
    char *argv[5];
    int argc = 0;
    
    argv[argc++] = "doom";
    argv[argc++] = "-iwad";
    argv[argc++] = "/wad/doom1.wad";

    if (storage_is_mounted()) {
        // We try to change directory to /sd so saves go there
        ESP_LOGI(TAG, "Setting working directory to /sd");
        chdir("/sd");
    }
    
    argv[argc] = NULL;
    
    // Check if WAD exists
    FILE *f = fopen("/wad/doom1.wad", "rb");
    if (!f) {
        ESP_LOGE(TAG, "doom1.wad not found! Trying fallbacks...");
        // Try other names?
        argv[2] = "/wad/doom.wad"; // Registered
    } else {
        fclose(f);
    }
    
    // Start DOOM
    // ONE WAY TICKET: This function loops forever until I_Quit called
    doom_main(argc, argv);
    
    // Should not reach here
    vTaskDelete(NULL);
}

// ============================================================================
//  Game Callbacks
// ============================================================================

static void doom_start(void) {
    ESP_LOGI(TAG, "Preparing for HELL...");
    
    // Try to mount SD Card
    if (storage_init() == ESP_OK) {
        ESP_LOGI(TAG, "SD Card ready for saves.");
    } else {
        ESP_LOGW(TAG, "No SD Card. Saves disabled.");
    }

    // Mount WAD filesystem
    if (!mount_wad_partition()) {
        ESP_LOGE(TAG, "Could not mount WAD partition!");
        
        display_fill_screen(0x0000); // Black
        display_set_cursor(10, 50);
        display_set_text_color(0xF800, 0x0000); // Red
        display_print("NO MEMORY OF HELL FOUND");
        display_set_cursor(10, 70);
        display_set_text_color(0xFFFF, 0x0000);
        display_print("Flash WAD to 'wad' part");
        vTaskDelay(pdMS_TO_TICKS(5000));
        return; 
    }
    
    // DOOM needs a huge stack.
    // We create a separate task for it pinned to a core.
    // Core 1 is usually app core.
    
    xTaskCreatePinnedToCore(
        doom_task,
        "doom_core",
        32 * 1024,      // 32KB stack (DOOM uses heap mainly)
        NULL,
        5,
        NULL,
        1
    );
}

static void doom_loop(void) {
    // This is called by the main game loop, but DOOM has its own loop.
    // We can just yield here or handle overlay UI if needed.
    // Since DOOM takes over via i_video, we don't need to do much here.
    vTaskDelay(pdMS_TO_TICKS(100));
}

static void doom_stop(void) {
    // Killing DOOM is messy
}

// ============================================================================
//  Game Definition
// ============================================================================

const game_def_t game_doom = {
    .name = "DOOM",
    .description = "Knee-Deep in the Dead",
    .icon = NULL,
    .start = doom_start,
    .loop = doom_loop,
    .stop = doom_stop,
    .requires_psram = true,
};
