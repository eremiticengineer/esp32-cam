#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "messages.h"

struct SystemBus {
    QueueHandle_t commandQueue;
    QueueHandle_t eventQueue;

    QueueHandle_t cameraQueue;
    QueueHandle_t sdQueue;
};
