#include "TcpServer.h"

#include "protocol/tcp/TcpWorker.h"
#include "protocol/tcp/TcpReactor.h"

TcpServer::TcpServer(int port, RxRouter* rxRouter, int worker, 
        ThreadManager* threadManager, std::shared_ptr<TlsServer> tlsServer)
{
    for (int i = 0; i < worker; ++i)
    {
        m_tcpWorkers.emplace_back(std::make_unique<TcpWorker>(rxRouter, threadManager, i));
    }
    m_tcpReactor = std::make_unique<TcpReactor>(port, m_tcpWorkers, tlsServer);
}

TcpServer::~TcpServer()
{
    stop();
}

void TcpServer::start()
{
    for (auto& worker : m_tcpWorkers)
    {
        worker->start();
    }
    m_tcpReactor->start();
}

void TcpServer::stop()
{
    m_tcpReactor->stop();
    for (auto& worker : m_tcpWorkers)
    {
        worker->stop();
    }
}

void TcpServer::enqueueTx(std::unique_ptr<Packet> pkt)
{
    if (not m_tcpReactor)
    {
        return;
    }

    m_tcpReactor->enqueueTx(std::move(pkt));
}

