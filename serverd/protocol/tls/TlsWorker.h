#pragma once

#include "algorithm/SpscQueue.h"

#include <memory>
#include <atomic>

class Epoll;
class RxRouter;
class Packet;
class ThreadManager;

class TlsWorker
{
public:
    TlsWorker(RxRouter* rxRouter, ThreadManager* threadManager, int id);
    ~TlsWorker();

    void start();
    void stop();

    void enqueueRx(std::unique_ptr<Packet> pkt);

private:
    void processPacket();

    RxRouter* m_rxRouter;
    ThreadManager* m_threadManager;

    std::atomic<bool> m_running{false};

    std::unique_ptr<SpscQueue<Packet>> m_rxQueue;
    std::unique_ptr<Epoll> m_rxEpoll;

    int m_id;
};
