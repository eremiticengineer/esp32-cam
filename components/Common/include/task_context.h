#pragma once

class ESP32Cam;
class SDCard;
class UartAPI;

struct TaskContext {
    ESP32Cam* camera;
    SDCard* sdcard;
    UartAPI* uart;
};
