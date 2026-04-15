#pragma once
#include <stdint.h>
#include <stddef.h>

struct ImageData {
    uint8_t* buffer;
    size_t length;
};

enum class SDCardEventType {
    TextData,
    BinaryData
};

struct SaveToSDPayload {
    char filename[32];
    SDCardEventType type;
    uint8_t* binary_buffer;
    char* text_buffer;
    size_t length;
};

enum class CommandType {
    TakePicture,
    SaveImageToSD,
    SendUART
};

struct Command {
    CommandType type;

    union {
        SaveToSDPayload sdcard_payload;
        ImageData image;
    };
};

enum class EventType {
    ImageCaptured,
    ImageSaved,
    Error
};

struct Event {
    EventType type;

    union {
        ImageData image;
        SaveToSDPayload sdcard_payload;
    };
};
