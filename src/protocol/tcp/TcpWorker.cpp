#include "TcpWorker.h"

#include "util/Logger.h"

TcpWorker::TcpWorker(RxRouter* rxRouter, int worker, ThreadManager* threadManager) :
    m_rxRouter(rxRouter),
    m_worker(worker),
    m_threadManager(threadManager)
{

}

TcpWorker::~TcpWorker()
{

}

void TcpWorker::start()
{
    m_running = true;    

    for (int i = 0; i < m_worker; ++i)
    {
        m_threadManager->addThread("tcp_worker_" + std::to_string(i),
                std::bind(&TcpWorker::processPacket, this),
                std::function<void()>{});
    }
}


void TcpWorker::stop()
{
    m_running = false;
}

void TcpWorker::processPacket()
{
    while(m_running)
    {
        LOG_DEBUG("Process packet");
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));    
    }
}
