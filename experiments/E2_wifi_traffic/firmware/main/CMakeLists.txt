idf_component_register(SRCS "sniffer.c" "cc1101.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES esp_wifi esp_event esp_timer nvs_flash esp_driver_usb_serial_jtag esp_driver_gpio esp_driver_spi)
