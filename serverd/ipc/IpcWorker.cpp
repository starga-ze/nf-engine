#include "ipc/IpcWorker.h"

#include "ipc/IpcMessage.h"
#include "ipc/IpcReactor.h"
#include "ipc/IpcCommandHandler.h"

#include "util/Logger.h"
#include "util/ThreadManager.h"
#include "io/Epoll.h"

#include <sys/epoll.h>
#include <functional>
#include <string>

#define IPC_SPSC_QUEUE_SIZE (65536)

IpcWorker::IpcWorker(IpcCommandHandler* handler,
                     ThreadManager* threadManager,
                     int id)
    : m_handler(handler),
      m_threadManager(threadManager),
      m_id(id)
{
    m_rxQueue = std::make_unique<SpscQueue<IpcMessage>>(IPC_SPSC_QUEUE_SIZE);
    m_rxEpoll = std::make_unique<Epoll>();
}

IpcWorker::~IpcWorker()
{
    stop();
}

void IpcWorker::start()
{
    if (!m_rxEpoll->init())
    {
        LOG_ERROR("IpcWorker rx epoll init failed");
        return;
    }

    m_running = true;

    m_threadManager->addThread(
        "ipc_worker_" + std::to_string(m_id),
        std::bind(&IpcWorker::processMessage, this),
        std::function<void()>{});
}

void IpcWorker::stop()
{
    m_running = false;
    if (m_rxEpoll)
        m_rxEpoll->wakeup();
}

void IpcWorker::setReactor(IpcReactor* reactor)
{
    m_reactor = reactor;
}

void IpcWorker::enqueueRx(std::unique_ptr<IpcMessage> msg)
{
    if (!msg)
        return;

    if (m_rxQueue->enqueue(std::move(msg)))
    {
        m_rxEpoll->wakeup();
    }
    else
    {
        LOG_WARN("IpcWorker SPSC Rx queue full, msg dropped");
    }
}

void IpcWorker::processMessage()
{
    std::vector<epoll_event> events(4);

    while (m_running)
    {
        int n = m_rxEpoll->wait(events, -1);
        if (n <= 0)
            continue;

        for (int i = 0; i < n; ++i)
        {
            if (events[i].data.fd != m_rxEpoll->getWakeupFd())
                continue;

            m_rxEpoll->drainWakeup();

            while (auto msg = m_rxQueue->dequeue())
            {
                const auto& p = msg->getPayload();
                if (p.size() < 2)
                {
                    continue;
                }
                std::string reqBody(reinterpret_cast<const char*>(p.data() + 2), p.size() - 2);

                std::string respBody;
                try
                {
                    respBody = m_handler->handle(reqBody);
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR("IPC handler exception: {}", e.what());
                    respBody = R"({"ok":false,"error":"handler exception"})";
                }
                catch (...)
                {
                    LOG_ERROR("IPC handler unknown exception");
                    respBody = R"({"ok":false,"error":"handler unknown exception"})";
                }

                auto reply = std::make_unique<IpcMessage>(msg->getFd(), std::move(respBody));
                m_reactor->enqueueTx(std::move(reply));
            }
        }
    }

    while (auto msg = m_rxQueue->dequeue())
    {
        const auto& p = msg->getPayload();
        if (p.size() < 2)
            continue;

        std::string reqBody(reinterpret_cast<const char*>(p.data() + 2), p.size() - 2);
        std::string respBody = m_handler->handle(reqBody);
        auto reply = std::make_unique<IpcMessage>(msg->getFd(), std::move(respBody));
        m_reactor->enqueueTx(std::move(reply));
    }
}
