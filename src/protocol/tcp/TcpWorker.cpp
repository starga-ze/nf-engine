#include "TcpWorker.h"

#include "util/Logger.h"
#include "util/ThreadManager.h"
#include "ingress/RxRouter.h"
#include "packet/Packet.h"

#include <thread>

TcpWorker::TcpWorker(RxRouter* rxRouter, int worker, ThreadManager* threadManager)
    : m_rxRouter(rxRouter),
      m_worker(worker),
      m_threadManager(threadManager)
{
}

TcpWorker::~TcpWorker()
{
    stop();
}

void TcpWorker::start()
{
    m_running = true;

    for (int i = 0; i < m_worker; ++i)
    {
        m_threadManager->addThread("tcp_worker_" + std::to_string(i),
            std::bind(&TcpWorker::processPacket, this),
            std::function<void()>{}
        );
    }
}

void TcpWorker::stop()
{
    m_running = false;
    m_cv.notify_all();
}

void TcpWorker::enqueueRx(std::unique_ptr<Packet> pkt)
{
    if (!pkt) return;

    {
        std::lock_guard<std::mutex> lock(m_lock);
        m_rxQueue.push(std::move(pkt));
    }
    m_cv.notify_one();
}

void TcpWorker::processPacket()
{
    while (true)
    {
        std::unique_ptr<Packet> pkt;

        {
            std::unique_lock<std::mutex> lock(m_lock);

            while (m_rxQueue.empty() and m_running)
            {
                m_cv.wait(lock);
            }

            if (!m_running && m_rxQueue.empty())
                return;

            pkt = std::move(m_rxQueue.front());
            m_rxQueue.pop();
        }

        m_rxRouter->handlePacket(std::move(pkt));
    }
}

