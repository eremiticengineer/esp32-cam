#pragma once

#include <string>
#include "esp_check.h"

class SDCard {
public:
    SDCard();
    ~SDCard();
    esp_err_t init(const char* mountPoint);
    esp_err_t writeFile(const char *path, char *data);
    esp_err_t readFile(const char *path, std::string& fileContent);

private:
};