#include "ipc/IpcReactor.h"

#include "ipc/IpcWorker.h"
#include "ipc/IpcMessage.h"
#include "ipc/IpcFraming.h"

#include "util/Logger.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/un.h>

#define IPC_MAX_EVENTS          (64)
#define IPC_RECV_CHUNK_SIZE     (4096)
#define IPC_MAX_RX_BUFFER_SIZE  (65536)
#define IPC_MAX_TX_BUFFER_SIZE  (65536)
#define IPC_MPSC_QUEUE_SIZE     (65536)

IpcReactor::IpcReactor(const std::string& socketPath,
        std::vector<std::unique_ptr<IpcWorker>>& ipcWorkers)
    : m_socketPath(socketPath),
    m_epoll(std::make_unique<Epoll>()),
    m_txQueue(std::make_unique<MpscQueue<IpcMessage>>(IPC_MPSC_QUEUE_SIZE))
{
    m_ipcWorkers.reserve(ipcWorkers.size());

    if (ipcWorkers.empty())
    {
        LOG_FATAL("IpcReactor requires at least one IpcWorker");
        std::abort();
    }

    for (const auto& worker : ipcWorkers)
        m_ipcWorkers.push_back(worker.get());
}

IpcReactor::~IpcReactor()
{
    stop();
}

void IpcReactor::start()
{
    if (!init())
        return;

    if (!m_epoll->init())
    {
        closeListen();
        return;
    }

    m_epoll->add(m_listenFd, EPOLLIN);

    m_running = true;

    std::vector<epoll_event> events(IPC_MAX_EVENTS);

    while (m_running)
    {
        int n = m_epoll->wait(events, -1);
        if (n < 0)
            continue;

        for (int i = 0; i < n; ++i)
        {
            int fd = events[i].data.fd;
            uint32_t ev = events[i].events;

            if (fd == m_epoll->getWakeupFd())
            {
                m_epoll->drainWakeup();
                processTxQueue();
                continue;
            }

            if (fd == m_listenFd)
            {
                acceptConnection();
                continue;
            }

            if (ev & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
            {
                closeConnection(fd);
                continue;
            }

            if (ev & EPOLLIN)
                receive(fd);

            if (ev & EPOLLOUT)
                flushTxBuffer(fd);
        }
    }

    shutdown();
}

void IpcReactor::stop()
{
    m_running = false;
    if (m_epoll)
        m_epoll->wakeup();
}

bool IpcReactor::init()
{
    if (!createSocket())
        return false;
    if (!bindAndListen())
        return false;

    LOG_INFO("IPC listen ready path={}", m_socketPath);
    return true;
}

bool IpcReactor::createSocket()
{
    ::unlink(m_socketPath.c_str());

    m_listenFd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_listenFd < 0)
    {
        LOG_ERROR("IPC socket() failed errno={}", errno);
        return false;
    }

    if (!setNonBlocking(m_listenFd))
    {
        LOG_ERROR("IPC setNonBlocking failed fd={}", m_listenFd);
        return false;
    }

    return true;
}

bool IpcReactor::bindAndListen()
{
    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;

    if (m_socketPath.size() >= sizeof(addr.sun_path))
    {
        LOG_ERROR("IPC socket path too long: {}", m_socketPath);
        return false;
    }

    std::strncpy(addr.sun_path, m_socketPath.c_str(), sizeof(addr.sun_path) - 1);

    if (::bind(m_listenFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0)
    {
        LOG_ERROR("IPC bind() failed path={} errno={}", m_socketPath, errno);
        return false;
    }

    if (::listen(m_listenFd, 64) != 0)
    {
        LOG_ERROR("IPC listen() failed errno={}", errno);
        return false;
    }

    return true;
}

void IpcReactor::closeListen()
{
    if (m_listenFd >= 0)
    {
        ::close(m_listenFd);
        m_listenFd = -1;
    }
}

bool IpcReactor::setNonBlocking(int fd)
{
    int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags < 0)
        return false;
    return ::fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

void IpcReactor::shutdown()
{
    for (auto& [fd, _] : m_conns)
        ::close(fd);
    m_conns.clear();

    if (m_epoll)
        m_epoll->close();

    closeListen();
    ::unlink(m_socketPath.c_str());
}

void IpcReactor::acceptConnection()
{
    while (true)
    {
        int fd = ::accept(m_listenFd, nullptr, nullptr);
        if (fd < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            return;
        }

        setNonBlocking(fd);

        auto conn = std::make_unique<Conn>(IPC_MAX_RX_BUFFER_SIZE, IPC_MAX_TX_BUFFER_SIZE);
        m_conns.emplace(fd, std::move(conn));

        m_epoll->add(fd, EPOLLIN | EPOLLRDHUP);
    }
}

void IpcReactor::closeConnection(int fd)
{
    m_epoll->del(fd);
    ::close(fd);
    m_conns.erase(fd);
}

void IpcReactor::receive(int fd)
{
    auto it = m_conns.find(fd);
    if (it == m_conns.end())
        return;

    auto& conn = it->second;
    ByteRingBuffer& rxRing = conn->rxRing;

    while (true)
    {
        uint8_t* writePtr = rxRing.writePtr();
        size_t writeLen = rxRing.writeLen();

        if (writeLen == 0)
        {
            LOG_WARN("IPC Rx Ring full, closing fd={}", fd);
            closeConnection(fd);
            return;
        }

        ssize_t r = ::recv(fd, writePtr, writeLen, 0);

        if (r > 0)
        {
            rxRing.produce(static_cast<size_t>(r));

            while (true)
            {
                size_t frameLen = 0;

                IpcFramingResult fr =
                    IpcFraming::tryExtractFrame(rxRing, frameLen);

                if (fr == IpcFramingResult::NeedMoreData)
                {
                    break;
                }

                if (fr != IpcFramingResult::Ok)
                {
                    closeConnection(fd);
                    return;
                }

                std::vector<uint8_t> payload(frameLen);

                size_t got = rxRing.read(payload.data(), frameLen);
                if (got != frameLen)
                {
                    LOG_FATAL("IPC RxRing Read invariant violation");
                    closeConnection(fd);
                    return;
                }

                size_t idx = m_rr.fetch_add(1) % m_ipcWorkers.size();
                auto msg = std::make_unique<IpcMessage>(fd, std::move(payload)); 

                m_ipcWorkers[idx]->enqueueRx(std::move(msg));
            }       
            continue;
        }

        if (r == 0)
        {
            closeConnection(fd);
            return;
        }

        if (errno == EINTR)
            continue;

        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;

        closeConnection(fd);
        return;
    }
}

void IpcReactor::enqueueTx(std::unique_ptr<IpcMessage> msg)
{
    if (!msg)
        return;

    if (m_txQueue->enqueue(std::move(msg)))
    {
        m_epoll->wakeup();
    }
    else
    {
        LOG_WARN("IpcReactor Tx MPSC queue full, msg dropped");
    }
}

void IpcReactor::processTxQueue()
{
    std::vector<std::unique_ptr<IpcMessage>> msgs;
    m_txQueue->dequeueAll(msgs);

    if (msgs.empty())
        return;

    std::unordered_set<int> dirtyFds;

    for (auto& msg : msgs)
    {
        int fd = msg->getFd();
        auto it = m_conns.find(fd);
        if (it == m_conns.end())
            continue;

        auto& conn = it->second;
        ByteRingBuffer& txRing = conn->txRing;

        const auto& payload = msg->getPayload();

        if (txRing.writable() < payload.size())
        {
            LOG_WARN("IPC Tx ring full, dropping reply fd={}", fd);
            continue;
        }

        txRing.write(payload.data(), payload.size());
        dirtyFds.insert(fd);
    }

    for (int fd : dirtyFds)
        flushTxBuffer(fd);
}

void IpcReactor::flushTxBuffer(int fd)
{
    auto it = m_conns.find(fd);
    if (it == m_conns.end())
        return;

    auto& conn = it->second;
    ByteRingBuffer& txRing = conn->txRing;

    if (txRing.readable() == 0)
        return;

    while (txRing.readable() > 0)
    {
        const uint8_t* ptr = txRing.readPtr();
        size_t len = txRing.readLen();

        ssize_t sent = ::send(fd, ptr, len, MSG_NOSIGNAL);

        if (sent < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                m_epoll->mod(fd, EPOLLIN | EPOLLOUT | EPOLLRDHUP);
                return;
            }

            LOG_ERROR("IPC send error fd={} errno={}", fd, errno);
            closeConnection(fd);
            return;
        }

        txRing.consume(static_cast<size_t>(sent));

        if (static_cast<size_t>(sent) < len)
        {
            m_epoll->mod(fd, EPOLLIN | EPOLLOUT | EPOLLRDHUP);
            return;
        }
    }

    closeConnection(fd);
}
