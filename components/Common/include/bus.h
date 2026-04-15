#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "messages.h"

struct SystemBus {
    QueueHandle_t commandQueue;
    QueueHandle_t eventQueue;
};
