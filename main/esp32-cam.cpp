#include "sdkconfig.h"

#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string.h>
#include "driver/i2c_master.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_camera.h"

#include "SDCard.h"

static const char *TAG = "ESP32CAM";

#define LOG_ERR(tag, err, msg) \
    ESP_LOGE(tag, "%s: %s (0x%x)", msg, esp_err_to_name(err), (unsigned int)(err))

// Pin definitions for CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

static camera_config_t camera_config;

void init_camera_config() {
  camera_config.ledc_channel = LEDC_CHANNEL_0;
  camera_config.ledc_timer = LEDC_TIMER_0;
  camera_config.pin_d0 = Y2_GPIO_NUM;
  camera_config.pin_d1 = Y3_GPIO_NUM;
  camera_config.pin_d2 = Y4_GPIO_NUM;
  camera_config.pin_d3 = Y5_GPIO_NUM;
  camera_config.pin_d4 = Y6_GPIO_NUM;
  camera_config.pin_d5 = Y7_GPIO_NUM;
  camera_config.pin_d6 = Y8_GPIO_NUM;
  camera_config.pin_d7 = Y9_GPIO_NUM;
  camera_config.pin_xclk = XCLK_GPIO_NUM;
  camera_config.pin_pclk = PCLK_GPIO_NUM;
  camera_config.pin_vsync = VSYNC_GPIO_NUM;
  camera_config.pin_href = HREF_GPIO_NUM;
  camera_config.pin_sccb_sda = SIOD_GPIO_NUM;
  camera_config.pin_sccb_scl = SIOC_GPIO_NUM;
  camera_config.pin_pwdn = PWDN_GPIO_NUM;
  camera_config.pin_reset = RESET_GPIO_NUM;
  camera_config.xclk_freq_hz = 20000000;
  camera_config.pixel_format = PIXFORMAT_JPEG; // Choices are YUV422, GRAYSCALE, RGB565, JPEG
  camera_config.frame_size = FRAMESIZE_UXGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
  camera_config.jpeg_quality = 10; //10-63 lower number means higher quality
  camera_config.fb_count = 2;
  camera_config.fb_location = CAMERA_FB_IN_PSRAM;
  camera_config.grab_mode = CAMERA_GRAB_LATEST;
}

static esp_err_t init_camera(void)
{
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Camera Init Failed");
        return err;
    }

    return ESP_OK;
}

extern "C" void app_main(void)
{
    init_camera_config();
    esp_err_t camera_err;
    camera_err = init_camera();
    if(ESP_OK != camera_err) {
      while (1) {
        LOG_ERR(TAG, camera_err, "cannot init camera");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
      }
    }

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
                ESP_LOGI(TAG, "%s", fileContent);
            }
        }
    }

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

        ESP_LOGI(TAG, "Taking picture...");
        camera_fb_t* pic = esp_camera_fb_get();
        if (pic != NULL) {
            ESP_LOGI(TAG, "Picture taken! Its size was: %zu bytes", pic->len);
            snprintf(filename, sizeof(filename), "/sdcard/image_%d.jpg", photoId++);
            ret = sdcard.write_binary_file(filename, pic->buf, pic->len);
            if (ESP_OK == ret) {
              ESP_LOGI(TAG, "wrote image to sdcard:");
              ESP_LOGI(TAG, "%s", filename);
            }
            else {
              LOG_ERR(TAG, ret, "cannot write image to sdcard");
            }
            esp_camera_fb_return(pic);
        }
        else {
          ESP_LOGE(TAG, "failed to take picture");
        }

        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}
