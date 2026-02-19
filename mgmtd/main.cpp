#include "ipc/UdsClient.h"
#include "service/StatsService.h"
#include "http/HttpServer.h"
#include "util/Logger.h"

#include <iostream>

int main() {
    Logger::Init("nf-mgmtd", "/var/log/nf/mgmtd.log", 1048576 * 5, 100);

    UdsClient ipc("/run/nf-server/nf-server.sock");
    StatsService svc(ipc);

    std::cout << "nf-mgmt service start...." << std::endl;    

    HttpServer server(8080, svc);
    server.run();

    Logger::Shutdown();

    return 0;
}

