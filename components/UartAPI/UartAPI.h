#include "bus.h"

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
};
