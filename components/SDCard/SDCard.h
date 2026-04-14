#pragma once

#include <string>
#include "esp_check.h"

class SDCard {
public:
    SDCard();
    ~SDCard();
    esp_err_t init_spi(const char* mountPoint);
    esp_err_t init_mmc(const char* mountPoint);
    esp_err_t write_file(const char *path, char *data);
    esp_err_t write_binary_file(const char *path, const uint8_t *data, size_t len);
    esp_err_t read_file(const char *path, std::string& fileContent);

private:
};
