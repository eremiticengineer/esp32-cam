#include "UartAPI.h"
#include "SDCard.h"

#include <esp_log.h>

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
      // err = uart_param_config(UART_NUM_0, &uart_config);
      // if (ESP_OK == err) {
      //   status = ESP_OK;
      //   ESP_LOGI(TAG, "UART ready");
      // }
      // else {
      //   status = err;
      //   LOG_ERR(TAG, err, "cannot config UART");
      // }
    }
    else {
      status = err;
      LOG_ERR(TAG, err, "cannot install UART driver");
    }

    /*
    esp_err_t err = uart_param_config(UART_NUM_0, &uart_config);
    if (ESP_OK == err) {
      uart_driver_install(UART_NUM_0, 2048, 0, 0, nullptr, 0);
      if (ESP_OK == err) {
        status = ESP_OK;
        ESP_LOGI(TAG, "UART ready");
      }
      else {
        status = err;
        LOG_ERR(TAG, err, "cannot install UART driver");
      }
    }
    else {
      status = err;
      LOG_ERR(TAG, err, "cannot config UART");
    }
  */

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
    static char rx_buffer[128];
    static int rx_index = 0;
    uint8_t c;

    while (1) {
      while (uart_read_bytes(UART_NUM_0, &c, 1, portMAX_DELAY)) {
        if (c == '#')
        {
          // terminate string so we can use strcmp
          rx_buffer[rx_index] = '\0';
          rx_index = 0;

          if (strcmp(rx_buffer, "ok") == 0) {
            uart_write_bytes(UART_NUM_0, "ok", 2);
          }
          else if (strcmp(rx_buffer, "pic:capture") == 0) {
            Command cmd;
            cmd.type = CommandType::TakePicture;
            xQueueSend(_bus->commandQueue, &cmd, portMAX_DELAY);
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
}  // void UartAPI::run() {

void UartAPI::event_listener() {
  Event event;
  while (true) {
    if (xQueueReceive(_bus->eventQueue, &event, portMAX_DELAY)) {
      if (event.type == EventType::ImageCaptured) {
        // Respond to the originating API call...
        const char *header = "IMG:";
        uart_write_bytes(UART_NUM_0, header, 4);
        uart_write_bytes(UART_NUM_0,
          (const char *)&event.image.length,
          sizeof(event.image.length));
        uart_write_bytes(UART_NUM_0,
          (const char *)event.image.buffer,
          event.image.length);

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
      } // if (event.type == EventType::ImageCaptured)
    } // if (xQueueReceive(_bus->eventQueue, &event, portMAX_DELAY)) {
  } // while (true) {
} // void UartAPI::event_listener() {
