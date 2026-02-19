#pragma once
#include "../service/StatsService.h"

class HttpServer {
public:
    HttpServer(int port, StatsService& svc);
    void run();

private:
    std::string loadFile(const std::string& path);

    int port_;
    StatsService& svc_;
};

