idf_component_register(
    SRCS "UartAPI.cpp"
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_driver_uart Common ESP32Cam SDCard
)
