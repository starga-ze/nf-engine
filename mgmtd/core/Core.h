#pragma once

#include "util/ThreadManager.h"

#include "ipc/IpcClient.h"
#include "ipc/IpcCommandHandler.h"

#include "service/AuthService.h"
#include "service/StatsService.h"

#include <memory>

class HttpServer;

class Core {
public:
    Core(int port);

    void run();

private:
    int m_port;
    std::shared_ptr<StatsService> m_statsService;
    std::shared_ptr<AuthService> m_authService;
    std::shared_ptr<HttpServer> m_httpServer;
    std::unique_ptr<ThreadManager> m_threadManager;
};

