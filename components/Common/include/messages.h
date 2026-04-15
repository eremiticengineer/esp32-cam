#pragma once
#include <stdint.h>
#include <stddef.h>

enum class CommandType {
    TakePicture,
    SaveImageToSD,
    SendUART
};

struct Command {
    CommandType type;

    // optional payload
    void* payload;
};

enum class EventType {
    ImageCaptured,
    ImageSaved,
    Error
};

struct ImageData {
    uint8_t* buffer;
    size_t length;
};

struct Event {
    EventType type;

    union {
        ImageData image;
    };
};
