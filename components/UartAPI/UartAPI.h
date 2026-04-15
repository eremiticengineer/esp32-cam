#include "bus.h"

class UartAPI {

public:
  UartAPI();
  void start(SystemBus* bus);
  void run();

private:
  SystemBus* _bus = nullptr;
  TaskHandle_t _taskHandle = nullptr;
  int _photo_id;

  static void task_wrapper(void* arg);
};
