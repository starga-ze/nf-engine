#pragma once

#include <memory>
#include <atomic>
#include <vector>

#include "packet/Packet.h"

class RxRouter;
class ThreadManager;
class SpscQueue;
class Epoll;

class UdpWorker
{
public:
    UdpWorker(RxRouter* rxRouter,
              ThreadManager* threadManager,
              int id);

    ~UdpWorker();

    void start();
    void stop();

    void enqueueRx(std::unique_ptr<Packet> pkt);

private:
    void processPacket();

private:
    RxRouter* m_rxRouter;
    ThreadManager* m_threadManager;

    int m_id;
    std::atomic<bool> m_running{false};

    std::unique_ptr<SpscQueue> m_rxQueue;
    std::unique_ptr<Epoll> m_rxEpoll;

};
