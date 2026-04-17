# esp32-cam
A simple esp-idf v6 project to work with an ESP32-CAM module. The project uses the ESPRESSIF esp32-camera component.

# Message bus and queues
The internal communication among the various components is handled by the Dispatcher component. The serial API sends messages on the command queue with the dispatcher routing the messages to the correct component to handle the request, e.g. the serial API tells the camera component to capture an image. The camera component then puts the image length and data on the event queue and the serial API listener picks up the event. It then sends a message on the command queue to tell the sdcard component to save the image to sdcard and another message to tell it to save text in a file on the sd card. Again, the dispatcher handles the routing to the sdcard, based on message type.

# Serial API
The app has a serial API that allows another mcu to power up the ESP32-CAM module and receive an image over UART serial connection. Commands are terminated with the hash char '#'. If an mcu wants to use the API it should ask the module if it's ready to accept API requests:

send:
```
ok#
```
receive:
```
ok
```
if ready for API requests

To tell the module to capture an image and return the bytes over serial,

send:
```
i:c#
```
receive a byte stream of the format:
```
IMG:<len><bytes>
```
The calling mcu should look for "IMG:", then read the next 4 bytes to get the length of the image data, then read those bytes.

## Testing the serial API
Plug in the module, e.g. using FTDI and run the script:
```
cd tools
python3 esp32camtest.py
```
If it works, the output looks like:
```
Probing ESP32...
ESP32 ready
Waiting for header...
Header found
Incoming data length: 283106
Saved to /tmp/image.jpg
```

# Adding esp32-camera dependency
```
cd components/ESP32Cam
idf.py add-dependency "espressif/esp32-camera"

cd components/SDCard
idf.py add-dependency sd-card
```

# Enable PSRAM in menuconfig
```
idf.py menuconfig
Component config -> ESP PSRAM -> Enable
Component config -> ESP PSRAM -> SPI RAM config -> Set RAM clock speed -> 80MHz (stuck on 40MHz)
```

# Switch MMC to 1 bit mode to prevent flash going off every time sdcard used
```
Component config -> SDCard configuration -> SDMMC bus width ... -> 1 line (D0)
```

# Resources
[esp32-camera](https://github.com/espressif/esp32-camera)
