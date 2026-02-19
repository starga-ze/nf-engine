#include "core/Core.h"
#include "service/StatsService.h"
#include "ipc/UdsClient.h"

#include <memory>
#include <iostream>

int main() {
    try {
        auto ipcClient = std::make_shared<UdsClient>("/run/nf-server/nf-server.sock");

        auto statsService = std::make_shared<StatsService>(ipcClient);

        Core core(8080, statsService);

        core.run();
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

