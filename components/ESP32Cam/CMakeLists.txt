idf_component_register(
    SRCS "ESP32Cam.cpp"
    INCLUDE_DIRS "."
    PRIV_REQUIRES nvs_flash
                  esp_psram
                  esp_driver_i2c
                  esp_driver_uart
)
