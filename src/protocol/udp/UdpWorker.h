#pragma once

#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>

class RxRouter;
class Packet;
class ThreadManager;

class UdpWorker
{
public:
    UdpWorker(RxRouter* rxRouter, int workerCount, ThreadManager* threadManager);
    ~UdpWorker();

    void start();
    void stop();

    void enqueueRx(std::unique_ptr<Packet> pkt);

private:
    void processPacket();

private:
    RxRouter* m_rxRouter{nullptr};
    int m_workerCount{0};
    ThreadManager* m_threadManager{nullptr};

    std::atomic<bool> m_running{false};

    std::mutex m_lock;
    std::condition_variable m_cv;
    std::queue<std::unique_ptr<Packet>> m_rxQueue;
};

