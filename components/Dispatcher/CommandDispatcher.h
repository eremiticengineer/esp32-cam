#include "bus.h"

class CommandDispatcher {
public:
    void start(SystemBus* bus);

private:
    static void task_wrapper(void* arg);
    void run();

    SystemBus* _bus;
};