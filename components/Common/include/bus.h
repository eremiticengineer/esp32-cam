#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "messages.h"

struct SystemBus {
    QueueHandle_t commandQueue;
    QueueHandle_t eventQueue;
    QueueHandle_t cameraQueue;
    QueueHandle_t sdQueue;

    void init() {
        commandQueue = xQueueCreate(10, sizeof(Command));
        eventQueue   = xQueueCreate(10, sizeof(Event));
        cameraQueue  = xQueueCreate(5, sizeof(Command));
        sdQueue      = xQueueCreate(5, sizeof(Command));
    }
};
