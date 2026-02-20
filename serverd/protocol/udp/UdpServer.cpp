#include "UdpServer.h"

#include "protocol/udp/UdpWorker.h"
#include "protocol/udp/UdpReactor.h"

UdpServer::UdpServer(int port, RxRouter* rxRouter, int worker, ThreadManager* threadManager)
{
    for (int i = 0; i < worker; ++i)
    {
        m_udpWorkers.emplace_back(std::make_unique<UdpWorker>(rxRouter, threadManager, i));
    }
    m_udpReactor = std::make_unique<UdpReactor>(port, m_udpWorkers);
}

UdpServer::~UdpServer()
{
    stop();
}

void UdpServer::start()
{
    for (auto& worker : m_udpWorkers)
    {
        worker->start();
    }
    m_udpReactor->start();
}

void UdpServer::stop()
{
    m_udpReactor->stop();
    for (auto& worker : m_udpWorkers)
    {
        worker->stop();
    }
}

void UdpServer::enqueueTx(std::unique_ptr<Packet> pkt)
{
    if (not m_udpReactor)
    {
        return;
    }

    m_udpReactor->enqueueTx(std::move(pkt));
}

