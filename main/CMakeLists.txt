idf_component_register(SRCS "esp32-cam.cpp"
                       PRIV_INCLUDE_DIRS "."
                       PRIV_REQUIRES nvs_flash
                                     SDCard
                                     ESP32Cam
                                     UartAPI
                                     Common
                                     Dispatcher
)
