#include "CommandDispatcher.h"

void CommandDispatcher::start(SystemBus* bus)
{
    _bus = bus;
    xTaskCreate(task_wrapper, "dispatcher", 4096, this, 5, nullptr);
}

void CommandDispatcher::task_wrapper(void* arg)
{
    static_cast<CommandDispatcher*>(arg)->run();
}

void CommandDispatcher::run()
{
    while (1)
    {
        Command cmd;
        xQueueReceive(_bus->commandQueue, &cmd, portMAX_DELAY);

        switch (cmd.type)
        {
            case CommandType::TakePicture:
                xQueueSend(_bus->cameraQueue, &cmd, portMAX_DELAY);
                break;

            case CommandType::SaveImageToSD:
                xQueueSend(_bus->sdQueue, &cmd, portMAX_DELAY);
                break;

            default:
                break;
        }
    }
}