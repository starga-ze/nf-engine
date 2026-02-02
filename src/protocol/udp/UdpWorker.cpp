#include "UdpWorker.h"

#include "util/ThreadManager.h"
#include "ingress/RxRouter.h"
#include "packet/Packet.h"
#include "util/Logger.h"

#include <functional>

UdpWorker::UdpWorker(RxRouter* rxRouter, int workerCount, ThreadManager* threadManager)
    : m_rxRouter(rxRouter),
      m_workerCount(workerCount),
      m_threadManager(threadManager)
{
}

UdpWorker::~UdpWorker()
{
    stop();
}

void UdpWorker::start()
{
    m_running = true;

    for (int i = 0; i < m_workerCount; ++i)
    {
        m_threadManager->addThread(
            "udp_worker_" + std::to_string(i),
            std::bind(&UdpWorker::processPacket, this),
            std::function<void()>{}
        );
    }
}

void UdpWorker::stop()
{
    m_running = false;
    m_cv.notify_all();
}

void UdpWorker::enqueueRx(std::unique_ptr<Packet> pkt)
{
    if (!pkt) return;

    {
        std::lock_guard<std::mutex> lock(m_lock);
        m_rxQueue.push(std::move(pkt));
    }
    m_cv.notify_one();
}

void UdpWorker::processPacket()
{
    while (true)
    {
        std::unique_ptr<Packet> pkt;

        {
            std::unique_lock<std::mutex> lock(m_lock);

            while (m_rxQueue.empty() and m_running)
                m_cv.wait(lock);

            if (!m_running && m_rxQueue.empty())
                return;

            pkt = std::move(m_rxQueue.front());
            m_rxQueue.pop();
        }
        m_rxRouter->handlePacket(std::move(pkt));
    }
}

