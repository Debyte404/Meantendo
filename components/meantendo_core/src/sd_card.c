/**
 * @file sd_card.c
 * @brief SD Card Helper Implementation
 */

#include "sd_card.h"
#include "meantendo_config.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"

static const char *TAG = "SD_CARD";
static sdmmc_card_t *s_card;
static bool s_mounted = false;

esp_err_t sd_card_mount(void) {
    if (s_mounted) return ESP_OK;

    ESP_LOGI(TAG, "Mounting SD Card at %s", SD_MOUNT_POINT);

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SD_SPI_HOST;

    // Use the custom SPI pins defined in configuration
    // Note: If SPI2 is used for display, we must ensure we claim the bus correctly
    // or use a separate bus if available/configured. 
    // In meantendo_config.h, we defined SD on SPI3 (VSPI).

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_PIN_MOSI,
        .miso_io_num = SD_PIN_MISO,
        .sclk_io_num = SD_PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    // Initialize the SPI bus for SD card
    // We assume this bus is NOT initialized yet. 
    // If Display and SD share a bus, we skip bus_initialize and just add device.
    // Here we check if HOST is different.
    
    // NOTE: spi_bus_initialize might fail if already init. 
    // We try to init, safely ignoring "already init" error if it happens but unlikely for VSPI.
    esp_err_t ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to initialize bus.");
        return ret;
    }

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_PIN_CS;
    slot_config.host_id = host.slot;

    ret = esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &host, &slot_config, &mount_config, &s_card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s).", esp_err_to_name(ret));
        }
        return ret;
    }

    ESP_LOGI(TAG, "SD Card mounted");
    sdmmc_card_print_info(stdout, s_card);
    s_mounted = true;
    return ESP_OK;
}

void sd_card_unmount(void) {
    if (!s_mounted) return;
    esp_vfs_fat_sdcard_unmount(SD_MOUNT_POINT, s_card);
    ESP_LOGI(TAG, "SD Card unmounted");
    s_mounted = false;
    s_card = NULL;
    // We probably shouldn't free the SPI bus if other things use it, 
    // but here we assume exclusive VSPI usage for SD.
    spi_bus_free(SD_SPI_HOST); 
}

bool sd_card_is_mounted(void) {
    return s_mounted;
}
