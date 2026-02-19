#include "ipc/UdsClient.h"
#include "service/StatsService.h"
#include "http/HttpServer.h"

#include <iostream>

int main() {
    UdsClient ipc("/run/nf-server/nf-server.sock");
    StatsService svc(ipc);

    std::cout << "nf-mgmt service start...." << std::endl;    

    HttpServer server(8080, svc);
    server.run();

    return 0;
}

