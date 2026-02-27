#include "Core.h"
#include "http/HttpServer.h"
#include "util/Logger.h"
#include "service/AuthService.h"

#include <thread>
#include <algorithm>

Core::Core(int port) : m_port(port)
{
}

void Core::run() 
{
    Logger::Init("nf-mgmtd", "/var/log/nf/mgmtd.log", 1048576 * 5, 100);
    
    auto ipcClient = std::make_shared<IpcClient>("/run/nf-server/nf-server.sock");
    auto ipcHandler = std::make_shared<IpcCommandHandler>(*ipcClient);

    m_threadManager = std::make_unique<ThreadManager>();

    m_statsService = std::make_shared<StatsService>(*ipcHandler);
    m_authService = std::make_shared<AuthService>();

    m_httpServer = std::make_shared<HttpServer>(m_port, m_statsService, m_authService);

    const size_t nThreads = 4;

    LOG_INFO("Mgmt daemon start...");

    m_threadManager->start(nThreads, "http_server", std::bind(&HttpServer::run, m_httpServer.get()));
            
    m_threadManager->join();

    Logger::Shutdown();
}

