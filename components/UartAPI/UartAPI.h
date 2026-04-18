#include "bus.h"

#include <string>

class UartAPI {

public:
  UartAPI();
  esp_err_t init();
  void start(SystemBus *bus);
  void run();
  void event_listener();

private:
  SystemBus* _bus = nullptr;
  TaskHandle_t _taskHandle = nullptr;
  TaskHandle_t _taskHandleEventListener = nullptr;
  int _photo_id;

  static void task_wrapper(void* arg);
  static void event_task_wrapper(void *arg);
  void process_uart_bytes(const uint8_t* input, size_t len);
  void on_command(const std::string& cmd);
  void on_response(const std::string& resp);
  void on_data(const uint8_t* data, size_t len);
};
