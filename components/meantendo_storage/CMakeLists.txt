idf_component_register(SRCS "src/storage.c"
                       INCLUDE_DIRS "include"
                       REQUIRES meantendo_core fatfs esp_driver_sdspi esp_driver_sdmmc_host)
