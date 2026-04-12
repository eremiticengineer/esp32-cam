# esp32-cam
A simple esp-idf v6 project to work with an ESP32-CAM module. The project uses the ESPRESSIF esp32-camera component.

# Adding esp32-camera dependency
```
cd main
idf.py add-dependency "espressif/esp32-camera"
```
this creates idf_component.yml in main.

# Enable PSRAM in menuconfig
```
idf.py menuconfig
Component config -> ESP PSRAM -> Enable
Component config -> ESP PSRAM -> SPI RAM config -> Set RAM clock speed -> 80MHz (stuck on 40MHz)
```

# Resources
[esp32-camera](https://github.com/espressif/esp32-camera)
