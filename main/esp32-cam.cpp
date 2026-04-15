#include "sdkconfig.h"

#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string.h>
#include "driver/i2c_master.h"
#include "driver/uart.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "SDCard.h"
#include "ESP32Cam.h"

static const char *TAG = "ESP32CAM";

#define LOG_ERR(tag, err, msg) \
    ESP_LOGE(tag, "%s: %s (0x%x)", msg, esp_err_to_name(err), (unsigned int)(err))

static TaskHandle_t uartTaskHandle;
static void uart_task(void *arg)
{
    uint8_t data[1024];

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

    while (1) {
        int len = uart_read_bytes(uart_num, data, 1024 - 1, pdMS_TO_TICKS(100));

        if (len > 0) {
            data[len] = 0; // null-terminate for string handling

            ESP_LOGI(TAG, "RECEIVED: %s", (char*)data);

            if (strcmp((char *)data, "pic:capture") == 0) {
                // trigger capture
                ESP_LOGI(TAG, "received!");
                //handle_capture();
            }
        }
    }
}

extern "C" void app_main(void)
{
    for (int i = 0; i < 5; i++) {
      ESP_LOGI(TAG, "Time elapsed: %d s", i + 1);
      vTaskDelay(pdMS_TO_TICKS(1000));
    }

    xTaskCreate(uart_task, "uart_task", 4096, NULL, 5, &uartTaskHandle);

    SDCard sdcard;
    esp_err_t ret = sdcard.init_mmc("/sdcard");
    if (ESP_OK != ret) {
        LOG_ERR(TAG, ret, "cannot mount sdcard");
    }
    else {
        char data[64];
        snprintf(data, 64, "this is a test file on the sdcard");
        ret = sdcard.write_file("/sdcard/test.txt", data);
        if (ESP_OK != ret) {
            LOG_ERR(TAG, ret, "cannot write file to  sdcard");
        }
        else {
            std::string fileContent;
            ret = sdcard.read_file("/sdcard/test.txt", fileContent);
            if (ESP_OK != ret) {
                LOG_ERR(TAG, ret, "cannot read file from  sdcard");
            }
            else {
                ESP_LOGI(TAG, "file content:");
                ESP_LOGI(TAG, "%s", fileContent.c_str());
            }
        }
    }

    ESP32Cam camera;
    camera.init_camera();

    int photoId = 1;
    char filename[32];
    while (1)
    {
        std::string fileContent;
        esp_err_t ret = sdcard.read_file("/sdcard/test.txt", fileContent);
        if (ESP_OK != ret) {
            LOG_ERR(TAG, ret, "cannot read file from  sdcard");
        }
        else {
            ESP_LOGI(TAG, "file content:");
            ESP_LOGI(TAG, "%s", fileContent.c_str());
        }

        ESP32Cam::ImageData image = camera.capture_image();
        if (image.data != nullptr) {
          ESP_LOGI(TAG, "Picture taken! Its size was: %zu bytes", image.len);
          snprintf(filename, sizeof(filename), "/sdcard/image_%d.jpg", photoId++);
          ret = sdcard.write_binary_file(filename, image.data, image.len);
          free(image.data);
          if (ESP_OK == ret) {
            ESP_LOGI(TAG, "wrote image to sdcard:");
            ESP_LOGI(TAG, "%s", filename);
          }
          else {
            LOG_ERR(TAG, ret, "cannot write image to sdcard");
          }
        }
        else {
          ESP_LOGE(TAG, "could not capture image");
        }

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}
