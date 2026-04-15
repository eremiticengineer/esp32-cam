#pragma once

#include "bus.h"
#include "messages.h"

#include <string>
#include "esp_check.h"

class SDCard {
public:
    SDCard();
    ~SDCard();
    void start(SystemBus* bus);
    esp_err_t init_spi(const char* mountPoint);
    esp_err_t init_mmc(const char* mountPoint);
    esp_err_t write_file(const char *path, char *data);
    esp_err_t write_binary_file(const char *path, const uint8_t *data, size_t len);
    esp_err_t read_file(const char *path, std::string& fileContent);
    const std::string& get_mount_point() const;

private:
    std::string _mountPoint;
    SystemBus* _bus = nullptr;
    TaskHandle_t _taskHandle = nullptr;
    QueueHandle_t _cmdQueue;
    QueueHandle_t _eventQueue;

    static void task_wrapper(void* arg);
    void run();
};
