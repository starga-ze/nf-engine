#pragma once

#include "algorithm/SpscQueue.h"

#include <atomic>
#include <memory>
#include <vector>

class ThreadManager;
class Epoll;

class IpcReactor;
class IpcCommandHandler;
class IpcMessage;

class IpcWorker
{
public:
    IpcWorker(IpcCommandHandler* handler,
              ThreadManager* threadManager,
              int id);
    ~IpcWorker();

    void start();
    void stop();
    
    void setReactor(IpcReactor* reactor);
    void enqueueRx(std::unique_ptr<IpcMessage> msg);

private:
    void processMessage();
    void drainRxQueue();

private:
    IpcCommandHandler* m_handler{nullptr}; // non-owning
    IpcReactor*        m_reactor{nullptr}; // non-owning

    ThreadManager* m_threadManager{nullptr};
    int m_id{0};

    std::atomic<bool> m_running{false};

    std::unique_ptr<SpscQueue<IpcMessage>> m_rxQueue;
    std::unique_ptr<Epoll> m_rxEpoll;
};
