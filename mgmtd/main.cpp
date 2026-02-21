#include "core/Core.h"
#include "service/StatsService.h"

#include "ipc/IpcClient.h"
#include "ipc/IpcCommandHandler.h"

#include <memory>

int main()
{
    auto ipcClient = std::make_shared<IpcClient>("/run/nf-server/nf-server.sock");
    auto ipcHandler = std::make_shared<IpcCommandHandler>(*ipcClient);
    auto statsService = std::make_shared<StatsService>(*ipcHandler);

    Core core(8080, statsService);
    core.run();

    return 0;
}
