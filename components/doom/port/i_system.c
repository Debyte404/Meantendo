/**
 * @file i_system.c
 * @brief DOOM System Interface for ESP32
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_system.h"

#include "doomdef.h"
#include "m_misc.h"
#include "i_video.h"
#include "i_sound.h"
#include "d_net.h"
#include "g_game.h"

#include "i_system.h"

static const char *TAG = "DOOM_SYS";

int mb_used = 4; // 4MB default for ESP32

void I_Tactile(int on, int off, int total) {
    // Not implemented
}

ticcmd_t emptycmd;
ticcmd_t* I_BaseTiccmd(void) {
    return &emptycmd;
}

int I_GetHeapSize(void) {
    return mb_used * 1024 * 1024;
}

byte* I_ZoneBase(int* size) {
    *size = mb_used * 1024 * 1024;
    
    // Try to allocate in PSRAM
    byte* mem = heap_caps_malloc(*size, MALLOC_CAP_SPIRAM);
    
    if (!mem) {
        ESP_LOGE(TAG, "Failed to allocate %d bytes in PSRAM", *size);
        // Fallback to smaller size if failed?
        *size = 2 * 1024 * 1024; // Try 2MB
        mem = heap_caps_malloc(*size, MALLOC_CAP_SPIRAM);
        if (!mem) {
            ESP_LOGE(TAG, "Failed to allocate 2MB in PSRAM");
            // DOOM cannot run without memory
             I_Error("Not enough PSRAM for DOOM");
        }
    }
    
    ESP_LOGI(TAG, "Allocated %d bytes in PSRAM for Zone", *size);
    return mem;
}

int I_GetTime(void) {
    // Return time in 1/35th of a second
    int64_t t_us = esp_timer_get_time();
    return (t_us * TICRATE) / 1000000;
}

void I_Init(void) {
    I_InitSound();
    // Graphics initialized in D_DoomMain -> I_InitGraphics
}

void I_Quit(void) {
    D_QuitNetGame();
    I_ShutdownSound();
    I_ShutdownMusic();
    M_SaveDefaults();
    I_ShutdownGraphics();
    
    ESP_LOGI(TAG, "DOOM Quit");
    // TODO: Return to menu instead of exit
    // For now, we just stop the task? 
    // This function is usually called on exit(0)
    vTaskDelete(NULL);
}

void I_WaitVBL(int count) {
    vTaskDelay(count * (1000 / 35) / portTICK_PERIOD_MS);
}

void I_BeginRead(void) {}
void I_EndRead(void) {}

byte* I_AllocLow(int length) {
    byte* mem = malloc(length); // Internal RAM is fine for small stuff
    if (!mem) {
         ESP_LOGE(TAG, "AllocLow failed for %d bytes", length);
         I_Error("Out of memory in AllocLow");
    }
    memset(mem, 0, length);
    return mem;
}

void I_Error(char *error, ...) {
    va_list argptr;
    va_start(argptr, error);
    ESP_LOGE(TAG, "DOOM Error: ");
    esp_log_writev(ESP_LOG_ERROR, TAG, error, argptr);
    ESP_LOGE(TAG, "\n");
    va_end(argptr);

    // D_QuitNetGame();
    // I_ShutdownGraphics();
    
    // Reboot? Or stick?
    ESP_LOGE(TAG, "System Halted.");
    while(1) { vTaskDelay(1000); }
}
