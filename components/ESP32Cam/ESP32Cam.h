#pragma once

#include "bus.h"
#include "messages.h"
#include <string>
#include "esp_check.h"

class ESP32Cam {
public:
    ESP32Cam();
    ~ESP32Cam();
    esp_err_t init_camera();
    ImageData capture_image();
    void start(SystemBus* bus);

private:
    SystemBus* _bus = nullptr;
    TaskHandle_t _taskHandle = nullptr;

    static void task_wrapper(void* arg);
    void run();
};
