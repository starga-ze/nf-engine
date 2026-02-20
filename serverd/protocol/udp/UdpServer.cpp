#include "UdpServer.h"

#include "protocol/udp/UdpWorker.h"
#include "protocol/udp/UdpReactor.h"

UdpServer::UdpServer(int port, RxRouter* rxRouter, int worker, ThreadManager* threadManager)
{
    for (int i = 0; i < worker; ++i)
    {
        m_udpWorker  = std::make_unique<UdpWorker>(rxRouter, threadManager, i);
    }
    m_udpReactor = std::make_unique<UdpReactor>(port, m_udpWorker.get());
}

UdpServer::~UdpServer()
{
    stop();
}

void UdpServer::start()
{
    m_udpWorker->start();
    m_udpReactor->start();
}

void UdpServer::stop()
{
    if (m_udpReactor) m_udpReactor->stop();
    if (m_udpWorker)  m_udpWorker->stop();
}

void UdpServer::enqueueTx(std::unique_ptr<Packet> pkt)
{
    if (not m_udpReactor) return;
    m_udpReactor->enqueueTx(std::move(pkt));
}

