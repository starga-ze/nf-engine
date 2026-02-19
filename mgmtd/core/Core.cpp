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

void Core::run() {
    m_httpServer = std::make_shared<HttpServer>(m_port, m_statsService);

    const size_t nThreads = std::max<size_t>(2, std::thread::hardware_concurrency());

    m_threadManager->start(nThreads, [this]() {
        m_httpServer->run();
    });

    m_threadManager->join();
}

