idf_component_register(SRCS "sniffer.c"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES esp_wifi esp_event esp_timer nvs_flash esp_driver_usb_serial_jtag)
