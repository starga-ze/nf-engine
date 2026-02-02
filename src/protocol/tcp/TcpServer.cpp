#include "TcpServer.h"

#include "protocol/tcp/TcpWorker.h"
#include "protocol/tcp/TcpReactor.h"

TcpServer::TcpServer(int port, RxRouter* rxRouter, int worker, 
        ThreadManager* threadManager, std::shared_ptr<TlsServer> tlsServer)
{
    m_tcpWorker = std::make_unique<TcpWorker>(rxRouter, worker, threadManager);
    m_tcpReactor = std::make_unique<TcpReactor>(port, m_tcpWorker.get(), tlsServer);
}

TcpServer::~TcpServer()
{
    stop();
}

void TcpServer::start()
{
    m_tcpWorker->start();
    m_tcpReactor->start();
}

void TcpServer::stop()
{
    m_tcpReactor->stop();
    m_tcpWorker->stop();
}
