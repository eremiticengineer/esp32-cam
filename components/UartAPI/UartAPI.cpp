#include "UartAPI.h"
#include "SDCard.h"

#include <esp_log.h>

#include <vector>
#include <algorithm>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"

static const char *TAG = "ESP32CAM";

#define LOG_ERR(tag, err, msg) \
    ESP_LOGE(tag, "%s: %s (0x%x)", msg, esp_err_to_name(err), (unsigned int)(err))

UartAPI::UartAPI() {
  _photo_id = 1;
}

esp_err_t UartAPI::init() {
    esp_err_t status;

    uart_config_t uart_config;
    uart_config.baud_rate = 115200;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity    = UART_PARITY_DISABLE;
    uart_config.stop_bits  = UART_STOP_BITS_1;
    uart_config.flow_ctrl  = UART_HW_FLOWCTRL_DISABLE;
    uart_config.source_clk = UART_SCLK_DEFAULT;
    uart_config.rx_glitch_filt_thresh = 0; // E (1573) uart: uart_param_config(1067): glitch filter on RX signal is not supported
    uart_config.rx_flow_ctrl_thresh = 0;

    esp_err_t err = uart_driver_install(UART_NUM_0, 2048, 0, 0, nullptr, 0);
    if (ESP_OK == err) {
      status = ESP_OK;
      ESP_LOGI(TAG, "UART ready");
    }
    else {
      status = err;
      LOG_ERR(TAG, err, "cannot install UART driver");
    }

    uart_param_config(UART_NUM_0, &uart_config);

    return status;
}

void UartAPI::start(SystemBus* bus) {
    _bus = bus;
    xTaskCreate(task_wrapper, "UartAPITask", 4096, this, 5, &_taskHandle);
    xTaskCreate(event_task_wrapper, "UartAPIListenerTask", 4096, this, 5, &_taskHandleEventListener);
}

void UartAPI::task_wrapper(void* arg) {
    UartAPI* self = static_cast<UartAPI*>(arg);
    self->run();
}

void UartAPI::event_task_wrapper(void* arg) {
    UartAPI* self = static_cast<UartAPI*>(arg);
    self->event_listener();
}

void UartAPI::run() {
    uint8_t rx_buf[128];

    while (true)
    {
        int len = uart_read_bytes(UART_NUM_0,
                                  rx_buf,
                                  sizeof(rx_buf),
                                  pdMS_TO_TICKS(20));

        if (len > 0)
        {
            process_uart_bytes(rx_buf, len);
        }
    }
}  // void UartAPI::run() {




void UartAPI::on_command(const std::string& cmd) {
  ESP_LOGI(TAG, "on_command %s", cmd.c_str());
  if (cmd == "ok") {
    uart_write_bytes(UART_NUM_0, "@ok@", 4);
  }
  else if (cmd == "i:c") {
    Command cmd;
    cmd.type = CommandType::TakePicture;
    xQueueSend(_bus->commandQueue, &cmd, portMAX_DELAY);
  }
}

void UartAPI::on_response(const std::string& resp) {
  ESP_LOGI(TAG, "on_response %s", resp.c_str());
}

void UartAPI::on_data(const uint8_t* data, size_t len) {
  ESP_LOGI(TAG, "on_data %d, %s", len, data);
}






static std::string buffer;
static std::string cmd;
static std::vector<uint8_t> data;

/*
 * command frame:
 * #ok#
 * #i:c#
 * response frame:
 * @ok@
 * data frame:
 * !94012!<binary bytes>
 */
void UartAPI::process_uart_bytes(const uint8_t* input, size_t len)
{
    // 1. append incoming bytes
    buffer.append(reinterpret_cast<const char*>(input), len);

    // safety: prevent runaway buffer if desync happens
    if (buffer.size() > 4096)
    {
        buffer.clear();
        return;
    }

    // 2. parse as many complete frames as possible
    while (true)
    {
        // -----------------------------
        // COMMAND FRAME: #...#
        // -----------------------------
        size_t cmd_start = buffer.find('#');

        // RESPONSE FRAME: @...@
        size_t resp_start = buffer.find('@');

        // DATA FRAME: !LEN!...
        size_t data_start = buffer.find('!');

        // choose earliest valid frame start
        size_t first = std::min({cmd_start, resp_start, data_start});

        if (first == std::string::npos)
            break;

        // erase garbage before frame start
        if (first > 0)
            buffer.erase(0, first);

        // refresh positions after trimming
        if (buffer.empty())
            break;

        char type = buffer[0];

        // -----------------------------
        // COMMAND FRAME
        // -----------------------------
        if (type == '#')
        {
            size_t end = buffer.find('#', 1);
            if (end == std::string::npos)
                break; // incomplete frame

            std::string cmd = buffer.substr(1, end - 1);
            buffer.erase(0, end + 1);

            on_command(cmd);
            continue;
        }

        // -----------------------------
        // RESPONSE FRAME
        // -----------------------------
        if (type == '@')
        {
            size_t end = buffer.find('@', 1);
            if (end == std::string::npos)
                break;

            std::string resp = buffer.substr(1, end - 1);
            buffer.erase(0, end + 1);

            on_response(resp);
            continue;
        }

        // -----------------------------
        // DATA FRAME: !LEN!<bytes>
        // -----------------------------
        if (type == '!')
        {
            size_t len_sep = buffer.find('!', 1);
            if (len_sep == std::string::npos)
                break;

            std::string len_str = buffer.substr(1, len_sep - 1);
            size_t data_len = std::strtoul(len_str.c_str(), nullptr, 10);

            size_t needed = len_sep + 1 + data_len;

            if (buffer.size() < needed)
                break; // wait for full payload

            const uint8_t* data_ptr =
                reinterpret_cast<const uint8_t*>(buffer.data() + len_sep + 1);

            on_data(data_ptr, data_len);

            buffer.erase(0, needed);
            continue;
        }

        // unknown byte → drop it (resync safety)
        buffer.erase(0, 1);
    }
}




void UartAPI::event_listener() {
  Event event;
  while (true) {
    if (xQueueReceive(_bus->eventQueue, &event, portMAX_DELAY)) {
      if (event.type == EventType::ImageCaptured) {

        char header[32];

        // convert length to ASCII
        int len = snprintf(header, sizeof(header), "!%u!", event.image.length);

        // send header
        uart_write_bytes(UART_NUM_0, header, len);

        // send image bytes
        uart_write_bytes(UART_NUM_0,
          (const char *)event.image.buffer,
          event.image.length);        
      


        /*
        // Respond to the originating API call, start with "!LEN!"
        const char *header = "!";
        uart_write_bytes(UART_NUM_0, header, 1);
        uart_write_bytes(UART_NUM_0,
          (const char *)&event.image.length,
          sizeof(event.image.length));
        uart_write_bytes(UART_NUM_0, header, 1);
        // ...then the bytes
        uart_write_bytes(UART_NUM_0,
          (const char *)event.image.buffer,
          event.image.length);
          */

        // ...save the image to sd card. The receiver will free the buffer
        Command cmd;
        cmd.type = CommandType::SaveImageToSD;
        snprintf(cmd.sdcard_payload.filename, sizeof(cmd.sdcard_payload.filename),
            "image_%d.jpg",
            _photo_id++);
        cmd.sdcard_payload.type = SDCardEventType::BinaryData;
        cmd.sdcard_payload.binary_buffer = event.image.buffer;
        cmd.sdcard_payload.length = event.image.length;
        xQueueSend(_bus->commandQueue, &cmd, portMAX_DELAY);

        // Send a text message to the sd card to have it stored on the sd card.
        // The receiver will free the buffer.
        cmd.type = CommandType::SaveTextToSD;
        snprintf(cmd.sdcard_payload.filename, sizeof(cmd.sdcard_payload.filename),
            "testfile.txt");
        cmd.sdcard_payload.type = SDCardEventType::TextData;
        char* payload = (char*)malloc(32);
        snprintf(payload, 32, "this is text");
        cmd.sdcard_payload.text_buffer = payload;
        cmd.sdcard_payload.length = strlen(payload);
        xQueueSend(_bus->commandQueue, &cmd, portMAX_DELAY);
      } // if (event.type == EventType::ImageCaptured)
    } // if (xQueueReceive(_bus->eventQueue, &event, portMAX_DELAY)) {
  } // while (true) {
} // void UartAPI::event_listener() {
