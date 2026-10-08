idf_component_register(SRCS "bringup.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES esp_lcd esp_driver_gpio esp_driver_spi esp_psram esp_timer spi_flash)
