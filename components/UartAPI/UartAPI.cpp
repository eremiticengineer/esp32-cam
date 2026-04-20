#include "UartAPI.h"

#include <vector>
#include <algorithm>

#include "driver/uart.h"
#include "driver/gpio.h"

static const char *TAG = "ESP32CAM";

uart_port_t _uart_num = UART_NUM_2;

#define LOG_ERR(tag, err, msg) \
    ESP_LOGE(tag, "%s: %s (0x%x)", msg, esp_err_to_name(err), (unsigned int)(err))

UartAPI::UartAPI() {
    _photo_id = 1;
}

esp_err_t UartAPI::init(int uart_num, int txPin, int rxPin) {
    _uart_num = static_cast<uart_port_t>(uart_num);

    esp_err_t status;

    uart_config_t uart_config = {};
    uart_config.baud_rate = 115200;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity    = UART_PARITY_DISABLE;
    uart_config.stop_bits  = UART_STOP_BITS_1;
    uart_config.flow_ctrl  = UART_HW_FLOWCTRL_DISABLE;
    uart_config.source_clk = UART_SCLK_DEFAULT;

    esp_err_t err = uart_driver_install(_uart_num, 2048, 2048, 0, nullptr, 0);
    if (ESP_OK == err) {
        err = uart_param_config(_uart_num, &uart_config);
        if (ESP_OK == err) {
            err = uart_set_pin(_uart_num,
                            txPin,
                            rxPin,
                            UART_PIN_NO_CHANGE,
                            UART_PIN_NO_CHANGE);
            gpio_set_pull_mode(static_cast<gpio_num_t>(rxPin), GPIO_PULLUP_ONLY);
            status = ESP_OK;
            ESP_LOGI(TAG, "UART ready");
            if (ESP_OK != err) {
                status = err;
                LOG_ERR(TAG, err, "cannot set UART pins");
            }
        }
        else {
            status = err;
            LOG_ERR(TAG, err, "cannot config UART");
        }
    }
    else {
      status = err;
      LOG_ERR(TAG, err, "cannot install UART driver");
    }

    return status;
}

void UartAPI::start(SystemBus* bus) {
    _bus = bus;
    xTaskCreate(event_listener_task_wrapper, "UartAPIEventListenerTask", 4096, this, 5, &_eventListenerTaskHandle);
    xTaskCreate(run_task_wrapper, "UartAPIRunTask", 4096, this, 5, &_runTaskHandle);
}

void UartAPI::event_listener_task_wrapper(void* arg) {
    UartAPI* self = static_cast<UartAPI*>(arg);
    self->event_listener();
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
        uart_write_bytes(_uart_num, header, len);

        // send image bytes
        uart_write_bytes(_uart_num,
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

void UartAPI::run_task_wrapper(void* arg) {
    UartAPI* self = static_cast<UartAPI*>(arg);
    self->run();
}
void UartAPI::run() {
    uint8_t rx_buf[128];

    while (true)
    {
        int len = uart_read_bytes(_uart_num,
                                  rx_buf,
                                  sizeof(rx_buf),
                                  pdMS_TO_TICKS(20));
        if (len > 0)
        {
            process_uart_bytes(rx_buf, len);
        }
    }
}  // void UartAPI::run() {

void UartAPI::request(const std::string& request) {
    int written = uart_write_bytes(_uart_num, request.c_str(), request.length());
    if (written < 0) {
        ESP_LOGE(TAG, "cannot write to uart");
    }
}

void UartAPI::on_command(const std::string& cmd) {
    ESP_LOGI(TAG, "on_command %s", cmd.c_str());
    if (cmd == "ok") {
        uart_write_bytes(_uart_num, "@ok@", 4);
    }
    else if (cmd == "i:c") {
        Command cmd;
        cmd.type = CommandType::TakePicture;
        xQueueSend(_bus->commandQueue, &cmd, portMAX_DELAY);
    }
    else if (cmd == "t") {
        Command cmd;
        cmd.type = CommandType::SaveTextToSD;
        snprintf(cmd.sdcard_payload.filename, sizeof(cmd.sdcard_payload.filename),
            "testing.txt");
        cmd.sdcard_payload.type = SDCardEventType::TextData;
        char* payload = (char*)malloc(32);
        snprintf(payload, 32, "0123456789");
        cmd.sdcard_payload.text_buffer = payload;
        cmd.sdcard_payload.length = strlen(payload);
        xQueueSend(_bus->commandQueue, &cmd, portMAX_DELAY);
    }
    else {
        uart_write_bytes(_uart_num, "@?@", 3);
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
    // Append incoming bytes
    buffer.append(reinterpret_cast<const char*>(input), len);

    // Hard safety cap
    if (buffer.size() > 4096)
    {
        buffer.clear();
        return;
    }

    while (true)
    {
        // Find next possible frame start
        size_t cmd = buffer.find('#');
        size_t resp = buffer.find('@');
        size_t data = buffer.find('!');

        size_t first = std::string::npos;

        if (cmd != std::string::npos) first = cmd;
        if (resp != std::string::npos) first = (first == std::string::npos) ? resp : std::min(first, resp);
        if (data != std::string::npos) first = (first == std::string::npos) ? data : std::min(first, data);

        // No frame markers at all → drop garbage safely
        if (first == std::string::npos)
        {
            buffer.clear();
            break;
        }

        // Drop leading garbage
        if (first > 0)
            buffer.erase(0, first);

        if (buffer.empty())
            break;

        char type = buffer[0];

        // =========================
        // COMMAND: #...#
        // =========================
        if (type == '#')
        {
            size_t end = buffer.find('#', 1);
            if (end == std::string::npos)
                break; // wait for full frame

            std::string cmd = buffer.substr(1, end - 1);
            buffer.erase(0, end + 1);

            on_command(cmd);
            continue;
        }

        // =========================
        // RESPONSE: @...@
        // =========================
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

        // =========================
        // DATA: !LEN!payload
        // =========================
        if (type == '!')
        {
            size_t sep = buffer.find('!', 1);
            if (sep == std::string::npos)
                break;

            std::string len_str = buffer.substr(1, sep - 1);

            // Validate length string
            if (len_str.empty())
            {
                buffer.erase(0, 1);
                continue;
            }

            size_t data_len = std::strtoul(len_str.c_str(), nullptr, 10);

            size_t needed = sep + 1 + data_len;

            if (buffer.size() < needed)
                break; // wait for full payload

            const uint8_t* data_ptr =
                reinterpret_cast<const uint8_t*>(buffer.data() + sep + 1);

            on_data(data_ptr, data_len);

            buffer.erase(0, needed);
            continue;
        }

        // =========================
        // Unknown byte → resync safely
        // =========================
        buffer.erase(0, 1);
    }
}