#include "sdkconfig.h"

#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string.h>

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

extern "C" void app_main(void)
{
    SystemBus bus;
    bus.init();

    vTaskDelay(5000 / portTICK_PERIOD_MS);

    // Set up the UART API which will use messages to coordinate activity
    UartAPI uartAPI;
    uartAPI.init();
    uartAPI.start(&bus);
    
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

    while (1) {
      vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}
