#include "TcpWorker.h"

#include "util/Logger.h"
#include "util/ThreadManager.h"
#include "ingress/RxRouter.h"
#include "packet/Packet.h"
#include "protocol/tcp/TcpEpoll.h"

#include <thread>
#include <sys/epoll.h>

#define TCP_SPSC_QUEUE_SIZE (65536) // 64K Slot

TcpWorker::TcpWorker(RxRouter* rxRouter, ThreadManager* threadManager, int id) : 
    m_rxRouter(rxRouter),
    m_threadManager(threadManager),
    m_id(id)
{
    m_rxQueue = std::make_unique<SpscQueue>(TCP_SPSC_QUEUE_SIZE);
    m_rxEpoll = std::make_unique<TcpEpoll>();
}

TcpWorker::~TcpWorker()
{
    stop();
}

void TcpWorker::start()
{
    if (not m_rxEpoll->init())
    {
        LOG_ERROR("TcpWorker rx epoll init failed");
        return;
    }

    m_running = true;

    m_threadManager->addThread("tcp_worker_" + std::to_string(m_id), 
            std::bind(&TcpWorker::processPacket, this),
            std::function<void()>{});
}

void TcpWorker::stop()
{
    m_running = false;
    m_rxEpoll->wakeup();
}

void TcpWorker::enqueueRx(std::unique_ptr<Packet> pkt)
{
    if (not pkt) 
    {
        return;
    }

    if (m_rxQueue->enqueue(std::move(pkt)))
    {
        m_rxEpoll->wakeup();
    }
    else
    {
        LOG_WARN("TcpWorker SPSC Rx queue full, pkt dropped");
    }
}

void TcpWorker::processPacket()
{
    std::vector<epoll_event> events(4);

    while (m_running)
    {
        int n = m_rxEpoll->wait(events, -1);
        if (n <= 0)
        {
            continue;
        }

        for (int i = 0; i < n; ++i)
        {
            if (events[i].data.fd == m_rxEpoll->getWakeupFd())
            {
                m_rxEpoll->drainWakeup();

                while (auto pkt = m_rxQueue->dequeue())
                {
                    m_rxRouter->handlePacket(std::move(pkt));
                }
            }
        }
    }

    while (auto pkt = m_rxQueue->dequeue())
    {
        m_rxRouter->handlePacket(std::move(pkt));
    }
}

