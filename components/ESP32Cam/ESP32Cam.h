#pragma once

#include <string>
#include "esp_check.h"

class ESP32Cam {
public:

    struct ImageData {
        uint8_t* data;
        size_t len;
    };

    ESP32Cam();
    ~ESP32Cam();
    esp_err_t init_camera();
    ImageData capture_image();

private:
};
