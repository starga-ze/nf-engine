#pragma once

#include "algorithm/SpscQueue.h"

#include <queue>
#include <memory>
#include <atomic>

class Epoll;
class RxRouter;
class Packet;
class ThreadManager;

class TcpWorker {
public:
    TcpWorker(RxRouter* rxRouter, ThreadManager* threadManager, int id);
    ~TcpWorker();

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

