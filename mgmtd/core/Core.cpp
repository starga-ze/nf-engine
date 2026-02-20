#include "Core.h"
#include "http/HttpServer.h"

#include <thread>
#include <algorithm>

Core::Core(int port, std::shared_ptr<StatsService> svc)
    : m_port(port),
      m_statsService(std::move(svc)),
      m_threadManager(std::make_unique<ThreadManager>())
{
}

void Core::run() 
{
    m_httpServer = std::make_shared<HttpServer>(m_port, m_statsService);

    const size_t nThreads = 4;

    m_threadManager->start(nThreads, std::bind(&HttpServer::run, m_httpServer.get()));
            
    m_threadManager->join();
}

