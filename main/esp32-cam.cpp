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

#include "bus.h"
#include "SDCard.h"
#include "ESP32Cam.h"
#include "UartAPI.h"
#include "CommandDispatcher.h"

static const char *TAG = "ESP32CAM";

#define LOG_ERR(tag, err, msg) \
    ESP_LOGE(tag, "%s: %s (0x%x)", msg, esp_err_to_name(err), (unsigned int)(err))


/*
static TaskHandle_t uartTaskHandle;
static void uart_task(void* param) {
    TaskContext* ctx = static_cast<TaskContext*>(param);
    ESP32Cam* camera = ctx->camera;
    SDCard* sdcard = ctx->sdcard;

    uint8_t data[1024];
    int photoId = 1;
    char filename[32];

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
          rx_buffer[rx_index] = '\0';   // terminate string
          rx_index = 0;

          if (strcmp(rx_buffer, "ok") == 0) {
            uart_write_bytes(uart_num, "ok", 2);
          }
          else if (strcmp(rx_buffer, "pic:capture") == 0) {
            //ESP_LOGI("UART", "***********************************************: %s", rx_buffer);
            ESP32Cam::ImageData image = camera->capture_image();

            const char *header = "IMG:";
            uart_write_bytes(uart_num, header, 4);
            uart_write_bytes(uart_num, (const char *)&image.len, sizeof(image.len));
            uart_write_bytes(uart_num, (const char *)image.data, image.len);
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
} // static void uart_task(void* param)
*/

extern "C" void app_main(void)
{
    SystemBus bus;

    // Command + event bus
    bus.commandQueue = xQueueCreate(10, sizeof(Command));
    bus.eventQueue = xQueueCreate(10, sizeof(Event));

    // Subsystem queues
    bus.cameraQueue = xQueueCreate(5, sizeof(Command));
    bus.sdQueue     = xQueueCreate(5, sizeof(Command));
    
    // Start the message dispatcher
    CommandDispatcher commandDispatcher;
    commandDispatcher.start(&bus);

    // Set up the camera task to listen for commands to capture images
    ESP32Cam camera;
    camera.init_camera();
    camera.start(&bus);

    SDCard sdcard;
    esp_err_t ret = sdcard.init_mmc("/sdcard");
    sdcard.start(&bus);

    // Set up the UART API which will use messages to coordinate activity
    UartAPI uartAPI;
    uartAPI.start(&bus);

/*
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
*/

/*
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
*/

        while (1) {
          vTaskDelay(5000 / portTICK_PERIOD_MS);
        }
//    }
}
