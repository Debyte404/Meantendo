/**
 * @file storage.c
 * @brief Meantendo Storage Implementation
 * @author Debyte
 */

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "esp_log.h"
#include "driver/sdspi_host.h"

#include "storage.h"
#include "meantendo_config.h"

static const char *TAG = "STORAGE";

static sdmmc_card_t *s_card = NULL;
static bool s_mounted = false;

esp_err_t storage_init(void) {
    if (s_mounted) {
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing SD Card...");

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    // Use SPI host
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SD_SPI_HOST;
    host.max_freq_khz = 20000; // 20 MHz safe limit

    // SPI Slot Config
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_PIN_CS;
    slot_config.host_id = SD_SPI_HOST;

    // IMPORTANT: The SPI bus must be initialized already by the display driver
    // or we must check if it needs initialization. 
    // Since we are sharing the bus, we assume the bus is already initialized 
    // or we might need to handle the bus init carefully if display isn't init yet.
    // For Meantendo, we'll assume Display initializes the bus, OR we check.
    // However, `esp_vfs_fat_sdspi_mount` expects the bus to be free or usable.
    // If the bus was initialized via `spi_bus_initialize` (which display.c does),
    // we just add the device.
    
    // Note: eps_vfs_fat_sdspi_mount internally calls sdspi_host_init_device
    // preventing us from easily using a shared bus if we don't be careful.
    // Actually, for shared bus, it is better to skip internal bus init.
    // But `esp_vfs_fat_sdspi_mount` tries to init the bus if not done? 
    // No, it takes `host_config` and `slot_config`.

    ESP_LOGI(TAG, "Mounting filesystem at %s", SD_MOUNT_POINT);

    esp_err_t ret = esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &host, &slot_config, &mount_config, &s_card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s).", esp_err_to_name(ret));
        }
        return ret;
    }

    // Print card info
    sdmmc_card_print_info(stdout, s_card);
    s_mounted = true;
    
    return ESP_OK;
}

esp_err_t storage_deinit(void) {
    if (!s_mounted) return ESP_OK;

    esp_err_t ret = esp_vfs_fat_sdcard_unmount(SD_MOUNT_POINT, s_card);
    if (ret == ESP_OK) {
        s_mounted = false;
        s_card = NULL;
        ESP_LOGI(TAG, "SD Card unmounted");
    }
    return ret;
}

bool storage_is_mounted(void) {
    return s_mounted;
}

uint32_t storage_get_total_space_mb(void) {
    if (!s_mounted || !s_card) return 0;
    return ((uint64_t)s_card->csd.capacity * s_card->csd.sector_size) / (1024 * 1024);
}

uint32_t storage_get_free_space_mb(void) {
    if (!s_mounted) return 0;
    FATFS *fs;
    DWORD fre_clust;
    if (f_getfree("0:", &fre_clust, &fs) != FR_OK) { // "0:" is default drive
        return 0;
    }
    return ((uint64_t)fre_clust * fs->csize * fs->ssize) / (1024 * 1024);
}
