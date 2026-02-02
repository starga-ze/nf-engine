#pragma once

#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>

class RxRouter;
class Packet;
class ThreadManager;

class TcpWorker {
public:
    TcpWorker(RxRouter* rxRouter, int worker, ThreadManager* threadManager);
    ~TcpWorker();

    void start();
    void stop();

    void enqueueRx(std::unique_ptr<Packet> pkt);

private:
    void processPacket();

private:
    RxRouter* m_rxRouter;
    int m_worker;
    ThreadManager* m_threadManager;

    std::atomic<bool> m_running{false};

    std::mutex m_lock;
    std::condition_variable m_cv;
    std::queue<std::unique_ptr<Packet>> m_rxQueue;
};

