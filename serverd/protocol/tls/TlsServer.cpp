#include "protocol/tls/TlsServer.h"

#include "protocol/tls/TlsWorker.h"
#include "protocol/tls/TlsReactor.h"

TlsServer::TlsServer(SSL_CTX* ctx, RxRouter* rxRouter, int workerCount, ThreadManager* threadManager)
{
    for (int i = 0; i < workerCount; ++i)
    {
        m_tlsWorkers.emplace_back(std::make_unique<TlsWorker>(rxRouter, threadManager, i));
    }

    m_tlsReactor = std::make_unique<TlsReactor>(ctx, m_tlsWorkers);
}

TlsServer::~TlsServer()
{
    stop();
}

void TlsServer::start()
{
    for (auto& w : m_tlsWorkers)
    {
        w->start();
    }
    m_tlsReactor->start();
}

void TlsServer::stop()
{
    if (m_tlsReactor)
    {
        m_tlsReactor->stop();
    }

    for (auto& w : m_tlsWorkers)
    {
        w->stop();
    }
}

void TlsServer::enqueueTx(std::unique_ptr<Packet> pkt)
{
    if (!m_tlsReactor)
    {
        return;
    }
    m_tlsReactor->enqueueTx(std::move(pkt));
}

void TlsServer::handleTlsConnection(int fd, std::pair<sockaddr_in, sockaddr_in> connInfo)
{
    if (!m_tlsReactor)
    {
        return;
    }
    m_tlsReactor->handover(fd, connInfo);
}
