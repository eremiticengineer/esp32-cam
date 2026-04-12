idf_component_register(SRCS "esp32-cam.cpp"
                       PRIV_INCLUDE_DIRS "."
                       PRIV_REQUIRES nvs_flash esp_psram esp_driver_i2c)
