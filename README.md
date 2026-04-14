# esp32-cam
A simple esp-idf v6 project to work with an ESP32-CAM module. The project uses the ESPRESSIF esp32-camera component.

# Adding esp32-camera dependency
```
cd main
idf.py add-dependency "espressif/esp32-camera"
idf.py add-dependency sd-card
```
this creates idf_component.yml in main.

# Enable PSRAM in menuconfig
```
idf.py menuconfig
Component config -> ESP PSRAM -> Enable
Component config -> ESP PSRAM -> SPI RAM config -> Set RAM clock speed -> 80MHz (stuck on 40MHz)
```

# Switch MMC to 1 bit mode to prevent flash going off every time sdcard used
```
Component config -> SDCard configuration -> SDMMC bus width ... -> 1 line (D0)
```

# Resources
[esp32-camera](https://github.com/espressif/esp32-camera)
