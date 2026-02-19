#pragma once
#include "../service/StatsService.h"

class HttpServer {
public:
    HttpServer(int port, StatsService& svc);
    void run();

private:
    int port_;
    StatsService& svc_;
};

