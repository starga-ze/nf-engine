#include "protocol/tls/TlsWorker.h"

#include "util/Logger.h"
#include "util/ThreadManager.h"
#include "ingress/RxRouter.h"
#include "packet/Packet.h"
#include "io/Epoll.h"

#include <sys/epoll.h>
#include <vector>

#define TLS_SPSC_QUEUE_SIZE (65536)

TlsWorker::TlsWorker(RxRouter* rxRouter, ThreadManager* threadManager, int id) :
    m_rxRouter(rxRouter),
    m_threadManager(threadManager),
    m_id(id)
{
    m_rxQueue = std::make_unique<SpscQueue>(TLS_SPSC_QUEUE_SIZE);
    m_rxEpoll = std::make_unique<Epoll>();
}

TlsWorker::~TlsWorker()
{
    stop();
}

void TlsWorker::start()
{
    if (not m_rxEpoll->init())
    {
        LOG_ERROR("TlsWorker rx epoll init failed");
        return;
    }

    m_running = true;

    m_threadManager->addThread("tls_worker_" + std::to_string(m_id),
            std::bind(&TlsWorker::processPacket, this),
            std::function<void()>{});
}

void TlsWorker::stop()
{
    m_running = false;
    if (m_rxEpoll)
    {
        m_rxEpoll->wakeup();
    }
}

void TlsWorker::enqueueRx(std::unique_ptr<Packet> pkt)
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
        LOG_WARN("TlsWorker SPSC Rx queue full, pkt dropped");
    }
}

void TlsWorker::processPacket()
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
