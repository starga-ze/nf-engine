#include "UdpWorker.h"

#include "ingress/RxRouter.h"
#include "util/ThreadManager.h"
#include "util/Logger.h"
#include "io/Epoll.h"
#include "algorithm/SpscQueue.h"

#include <sys/epoll.h>

#define UDP_SPSC_QUEUE_SIZE (65536) // 64K Slot

UdpWorker::UdpWorker(RxRouter* rxRouter,
                     ThreadManager* threadManager,
                     int id)
    : m_rxRouter(rxRouter),
      m_threadManager(threadManager),
      m_id(id)
{
    m_rxQueue = std::make_unique<SpscQueue>(UDP_SPSC_QUEUE_SIZE);
    m_rxEpoll = std::make_unique<Epoll>();
}

UdpWorker::~UdpWorker()
{
    stop();
}

void UdpWorker::start()
{
    if (!m_rxEpoll->init())
    {
        LOG_ERROR("UdpWorker epoll init failed");
        return;
    }

    m_running = true;

    m_threadManager->addThread(
        "udp_worker_" + std::to_string(m_id),
        std::bind(&UdpWorker::processPacket, this),
        std::function<void()>{}
    );
}

void UdpWorker::stop()
{
    m_running = false;

    if (m_rxEpoll)
    {
        m_rxEpoll->wakeup();
    }
}

void UdpWorker::enqueueRx(std::unique_ptr<Packet> pkt)
{
    if (!pkt)
        return;

    if (m_rxQueue->enqueue(std::move(pkt)))
    {
        m_rxEpoll->wakeup();
    }
    else
    {
        LOG_WARN("UdpWorker SPSC Rx queue full, pkt dropped");
    }
}

void UdpWorker::processPacket()
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
