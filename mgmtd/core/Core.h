#pragma once
#include <memory>
#include "../util/ThreadManager.h"

class HttpServer;
class StatsService;

class Core {
public:
    Core(int port, std::shared_ptr<StatsService> svc);
    void run();

private:
    int m_port;
    std::shared_ptr<StatsService> m_statsService;
    std::shared_ptr<HttpServer> m_httpServer;
    std::unique_ptr<ThreadManager> m_threadManager;
};

