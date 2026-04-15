#include "ESP32Cam.h"

#include "esp_camera.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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

static const char *TAG = "ESP32CAM";
static camera_config_t camera_config;

ESP32Cam::ESP32Cam() {}

ESP32Cam::~ESP32Cam() {}

esp_err_t ESP32Cam::init_camera() {
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

  esp_err_t err = esp_camera_init(&camera_config);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Camera Init Failed");
    return err;
  }

  return ESP_OK;
}

ImageData ESP32Cam::capture_image() {
  camera_fb_t* fb = esp_camera_fb_get();
  if (fb != NULL) {
    ESP_LOGI(TAG, "Image captured! Its size was: %zu bytes", fb->len);

    ImageData img;
    img.length = fb->len;

    // COPY OUT of camera memory to the heap
    img.buffer = (uint8_t*)malloc(fb->len);
    memcpy(img.buffer, fb->buf, fb->len);

    esp_camera_fb_return(fb);   // SAFE now

    return img;
  }
  else {
    return {nullptr, 0};
  }
}

void ESP32Cam::start(SystemBus* bus)
{
    _bus = bus;

    xTaskCreate(
        task_wrapper,
        "cam_task",
        4096,
        this,
        5,
        &_taskHandle
    );
}

void ESP32Cam::task_wrapper(void* arg)
{
    ESP32Cam* self = static_cast<ESP32Cam*>(arg);
    self->run();
}

void ESP32Cam::run()
{
    Command cmd;

    while (true)
    {
        // Does a task somewhere want an image captured?
        if (xQueueReceive(_bus->cameraQueue, &cmd, portMAX_DELAY))
        {
            if (cmd.type == CommandType::TakePicture)
            {
                // Reusable img, the original has been released back to the camera
                // Caller is responsible for freeing it.
                ImageData img = capture_image();

                Event event;
                event.type = EventType::ImageCaptured;
                event.image = img;

                // Send the image captured event to the event queue
                xQueueSend(_bus->eventQueue, &event, portMAX_DELAY);
            }
        }
    }
}
