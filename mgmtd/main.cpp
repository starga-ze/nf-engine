#include "core/Core.h"
#include "service/StatsService.h"
#include "ipc/UdsClient.h"

#include <memory>
#include <iostream>

int main() {
    auto ipcClient = std::make_shared<UdsClient>("/run/nf-server/nf-server.sock");

    auto statsService = std::make_shared<StatsService>(ipcClient);

    Core core(8080, statsService);
    core.run();
       
    return 0;
}

