#include "UartAPI.h"
#include "SDCard.h"

#include <esp_log.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"

static const char *TAG = "ESP32CAM";

#define LOG_ERR(tag, err, msg) \
    ESP_LOGE(tag, "%s: %s (0x%x)", msg, esp_err_to_name(err), (unsigned int)(err))

UartAPI::UartAPI() {}

void UartAPI::start(SystemBus* bus) {
    _bus = bus;
    xTaskCreate(task_wrapper, "UartAPITask", 4096, this, 5, &_taskHandle);
}

void UartAPI::task_wrapper(void* arg) {
    UartAPI* self = static_cast<UartAPI*>(arg);
    self->run();
}

void UartAPI::run() {
    const uart_port_t uart_num = UART_NUM_0;

    uart_config_t uart_config;
    uart_config.baud_rate = 115200;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity    = UART_PARITY_DISABLE;
    uart_config.stop_bits  = UART_STOP_BITS_1;
    uart_config.flow_ctrl  = UART_HW_FLOWCTRL_DISABLE;
    uart_config.source_clk = UART_SCLK_DEFAULT;
    uart_config.rx_glitch_filt_thresh = 0;

    esp_err_t err = uart_param_config(uart_num, &uart_config);
    uart_driver_install(uart_num, 2048, 0, 0, nullptr, 0);
    if (ESP_OK != err) {
      LOG_ERR(TAG, err, "cannot config UART");
      // ESP_ERROR_CHECK(uart_param_config(UART_NUM_1, &uart_config));
      vTaskDelete(NULL);
    }
    ESP_LOGI(TAG, "UART ready");

    static char rx_buffer[128];
    static int rx_index = 0;
    uint8_t c;

    while (1) {
      while (uart_read_bytes(uart_num, &c, 1, portMAX_DELAY)) {
        if (c == '#') {
          // terminate string so we can use strcmp
          rx_buffer[rx_index] = '\0';
          rx_index = 0;

          if (strcmp(rx_buffer, "ok") == 0) {
            uart_write_bytes(uart_num, "ok", 2);
          }
          else if (strcmp(rx_buffer, "pic:capture") == 0) {
            Command cmd;
            cmd.type = CommandType::TakePicture;
            cmd.payload = nullptr;
            xQueueSend(_bus->commandQueue, &cmd, portMAX_DELAY);

            // Listen for events
            Event event;
            if (xQueueReceive(_bus->eventQueue, &event, portMAX_DELAY))
            {
                if (event.type == EventType::ImageCaptured)
                {
                  const char *header = "IMG:";
                  uart_write_bytes(uart_num, header, 4);
                  uart_write_bytes(uart_num,
                    (const char *)&event.image.length,
                    sizeof(event.image.length));
                  uart_write_bytes(uart_num,
                    (const char *)event.image.buffer,
                    event.image.length);
                  free(event.image.buffer);
                }
            }


/*
            snprintf(filename, sizeof(filename), "%s/image_%d.jpg", sdcard->get_mount_point().c_str(), photoId++);
            err = sdcard->write_binary_file(filename, image.data, image.len);
            if (ESP_OK == err) {
              ESP_LOGI(TAG, "wrote image to sdcard:");
              ESP_LOGI(TAG, "%s", filename);
            }
            else {
              LOG_ERR(TAG, err, "cannot write image to sdcard");
            }
            free(image.data);
*/

          }
        } // else if (strcmp(rx_buffer, "pic:capture") == 0)
        else {
          if (rx_index < sizeof(rx_buffer) - 1) {
              rx_buffer[rx_index++] = c;
          }
          else {
          // overflow safety reset
          rx_index = 0;
        }
      } // if (c == '#')
    } // while (uart_read_bytes(uart_num, &c, 1, portMAX_DELAY))
  } // while (1)
}  // static void uart_task(void* param)
