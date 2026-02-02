#pragma once

#include "ingress/RxRouter.h"
#include "util/ThreadManager.h"

#include <atomic>

class TcpWorker
{
public:
    TcpWorker(RxRouter* rxRouter, int worker, ThreadManager* threadManager);
    ~TcpWorker();

    void start();
    void stop();

private:
    void processPacket();

    RxRouter* m_rxRouter;
    ThreadManager* m_threadManager;

    int m_worker;
    std::atomic<bool> m_running = false;
};
