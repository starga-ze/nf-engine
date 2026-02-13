#include "TcpReactor.h"

#include "protocol/tcp/TcpWorker.h"
#include "protocol/tls/TlsServer.h"
#include "packet/Packet.h"

#include "util/Logger.h"

#include <unistd.h> 
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define TCP_RECV_CHUNK_SIZE     (4096)
#define TCP_MAX_EVENTS          (64)
#define TCP_MAX_RX_BUFFER_SIZE  (65536) // 64 KB
#define TCP_MAX_TX_BUFFER_SIZE  (65536) // 64 KB
#define TCP_MPSC_QUEUE_SIZE (65536) // 64K Slot

TcpReactor::TcpReactor(int port, std::vector<std::unique_ptr<TcpWorker>>& tcpWorkers, 
        std::shared_ptr<TlsServer> tlsServer) :
    m_port(port),
    m_tlsServer(tlsServer)
{
    m_tcpWorkers.reserve(tcpWorkers.size());

    for (const auto& worker : tcpWorkers)
    {
        m_tcpWorkers.push_back(worker.get());
    }

    m_txQueue = std::make_unique<MpscQueue>(TCP_MPSC_QUEUE_SIZE);
    m_tcpEpoll = std::make_unique<TcpEpoll>();
}

TcpReactor::~TcpReactor()
{
    stop();
}

void TcpReactor::start()
{
    if (not init())
    {
        return;
    }
    
    if (not m_tcpEpoll->init())
    {
        close();
        return;
    }

    m_tcpEpoll->add(m_listenFd, EPOLLIN);

    m_running = true;

    std::vector<epoll_event> events(TCP_MAX_EVENTS);

    while (m_running) 
    {
        int n = m_tcpEpoll->wait(events, -1);
        if (n < 0) 
        {
            continue;
        }

        for (int i = 0; i < n; ++i) 
        {
            int fd = events[i].data.fd;
            uint32_t ev = events[i].events;

            if (fd == m_tcpEpoll->getWakeupFd()) 
            {
                m_tcpEpoll->drainWakeup();
                processTxQueue();
                continue;
            }

            if (fd == m_listenFd) 
            {
                acceptConnection();
                continue;
            }
    
            if(ev & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
            {
                closeConnection(fd);
                continue;
            }

            if (ev & EPOLLIN)
            {
                receive(fd);
            }

            if (ev & EPOLLOUT)
            {
                flushTxBuffer(fd);
            }
        }
    }

    shutdown();
    return;
}

void TcpReactor::stop()
{
    m_running = false;
    m_tcpEpoll->wakeup();
}

void TcpReactor::shutdown()
{
    for(auto& [fd, conn] : m_conns)
    {
        ::close(fd);
    }
    m_conns.clear();

    m_tcpEpoll->close();
    close();
}

bool TcpReactor::init()
{
    if (not create())
    {
        return false;
    }

    if (not setSockOpt())
    {
        return false;
    }

    if (not bindAndListen())
    {
        return false;
    }

    LOG_INFO("TCP listen ready port={}", m_port);
    return true;    
}

bool TcpReactor::create()
{
    m_listenFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (m_listenFd < 0)
    {
        LOG_ERROR("socket() failed errno={}", errno);
        return false;
    }

    if (not setNonBlocking(m_listenFd))
    {
        LOG_ERROR("setNonBlocking failed, fd={}", m_listenFd);
        return false;
    }

    return true;
}

bool TcpReactor::setSockOpt()
{
    int opt = 1;
    if (::setsockopt(m_listenFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0) {
        LOG_ERROR("setsockopt(SO_REUSEADDR) failed fd={}", m_listenFd);
        return false;
    }
    return true;
}

bool TcpReactor::bindAndListen()
{
    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_addr.s_addr = INADDR_ANY;
    m_serverAddr.sin_port = htons(m_port);

    if (::bind(m_listenFd, reinterpret_cast<sockaddr*>(&m_serverAddr), 
                sizeof(m_serverAddr)) != 0) 
    {
        LOG_ERROR("bind() failed port={} errno={}", m_port, errno);
        return false;
    }

    if (::listen(m_listenFd, SOMAXCONN) != 0) 
    {
        LOG_ERROR("listen() failed fd={} errno={}", m_listenFd, errno);
        return false;
    }

    return true;
}

void TcpReactor::close()
{
    if (m_listenFd >= 0)
    {
        ::close(m_listenFd);
        m_listenFd = -1;
    }
}

bool TcpReactor::setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
    {
        return false;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

void TcpReactor::acceptConnection()
{
    while (true) {
        sockaddr_in peer{};
        socklen_t len = sizeof(peer);

        int fd = ::accept(m_listenFd,
                          reinterpret_cast<sockaddr*>(&peer),
                          &len);
        if (fd < 0) 
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                break;
            }
            return;
        }

        setNonBlocking(fd);

        size_t rxCapacity = TCP_MAX_RX_BUFFER_SIZE;
        size_t txCapacity = TCP_MAX_TX_BUFFER_SIZE;

        ::setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rxCapacity, sizeof(rxCapacity));

        auto conn = std::make_unique<TcpConnection>(fd, peer, rxCapacity, txCapacity);

        m_conns.emplace(fd, std::move(conn));
        m_tcpEpoll->add(fd, EPOLLIN | EPOLLRDHUP);
    }
}

void TcpReactor::closeConnection(int fd)
{
    m_tcpEpoll->del(fd);
    ::close(fd);
    m_conns.erase(fd);
}

void TcpReactor::receive(int fd)
{
    if (m_tlsServer and isTlsClientHello(fd))
    {
        handoverToTls(fd);
        return;
    }

    auto it = m_conns.find(fd);
    if (it == m_conns.end())
        return;

    auto& conn = it->second;
    ByteRingBuffer& rxRing = conn->rxRing();

    while (true) {
        uint8_t* writePtr = rxRing.writePtr();
        size_t writeLen = rxRing.writeLen();

        if (writeLen == 0)
        {
            LOG_WARN("TCP Rx Ring is full, may cause backpressure");
            closeConnection(fd);
            return;
        }
    
        ssize_t wBytes = ::recv(fd, writePtr, writeLen, 0);

        if (wBytes > 0) {
            rxRing.produce(static_cast<size_t>(wBytes));

            while (true) {
                size_t frameLen = 0;

                TcpFramingResult r = TcpFraming::tryExtractFrame(rxRing, frameLen);

                if (r == TcpFramingResult::NeedMoreData) 
                {
                    break; 
                }
                if (r != TcpFramingResult::Ok)
                {
                    closeConnection(fd);
                    return;
                }

                std::vector<uint8_t> payload(frameLen);
                size_t rBytes = rxRing.read(payload.data(), frameLen);

                if (rBytes != frameLen)
                {
                    LOG_FATAL("Tcp RxRing Read Error, invariant violation");
                    closeConnection(fd);
                    return;
                }
                auto pkt = std::make_unique<Packet>(fd, Protocol::TCP, std::move(payload), 
                        conn->peer(), m_serverAddr);

                /* temp load balancing */
                size_t idx = pkt->getFd() % m_tcpWorkers.size();
                m_tcpWorkers[idx]->enqueueRx(std::move(pkt));
            }
            continue;
        }
        else {
            if (wBytes == 0) 
            {
                closeConnection(fd);
                return;
            }

            if (errno == EINTR)
            {
                continue;
            }
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                return;
            }

            closeConnection(fd);
            return;
        }
    }
}

bool TcpReactor::isTlsClientHello(int fd)
{
    uint8_t buf[5];
    ssize_t n = ::recv(fd, buf, sizeof(buf), MSG_PEEK);
    if (n < 5)
        return false;

    return buf[0] == 0x16 && buf[1] == 0x03;
}

void TcpReactor::handoverToTls(int fd)
{
    auto it = m_conns.find(fd);
    if (it == m_conns.end()) 
    {
        closeConnection(fd);
        return;
    }

    sockaddr_in peer = it->second->peer();

    m_tcpEpoll->del(fd);
    m_conns.erase(it);

    m_tlsServer->handleTlsConnection(fd, std::make_pair(m_serverAddr, peer));
}

void TcpReactor::enqueueTx(std::unique_ptr<Packet> pkt)
{
    if (!pkt) 
    {
        return;
    }
        
    if (m_txQueue->enqueue(std::move(pkt)))
    {
        m_tcpEpoll->wakeup();
    }
    else
    {
        LOG_WARN("TcpReactor Tx MPSC Queue Full, pkt dropped");
    }
}

void TcpReactor::processTxQueue()
{
    std::vector<std::unique_ptr<Packet>> packets;
    
    m_txQueue->dequeueAll(packets);

    if (packets.empty())
    {
        return;
    }

    std::unordered_set<int> dirtyFds;

    for (auto& pkt : packets)
    {
        int fd = pkt->getFd();
        auto it = m_conns.find(fd);
        if (it == m_conns.end())
        {
            continue;
        }

        auto& conn = it->second;
        ByteRingBuffer& txRing = conn->txRing();
        const auto& payload = pkt->getPayload();

        if (txRing.writable() < payload.size())
        {
            LOG_WARN("Tx RingBuffer Full, dropping packet, fd={}", fd);
            continue;
        }

        txRing.write(payload.data(), payload.size());

        dirtyFds.insert(fd);
    }

    for (int fd : dirtyFds)
    {
        flushTxBuffer(fd);
    }
}

void TcpReactor::flushTxBuffer(int fd)
{
    auto it = m_conns.find(fd);
    if (it == m_conns.end())
    {
        return;
    }

    auto& conn = it->second;
    ByteRingBuffer& txRing = conn->txRing();

    if (txRing.readable() == 0)
    {
        return;
    }

    while (txRing.readable() > 0)
    {
        const uint8_t* ptr = txRing.readPtr();
        size_t len = txRing.readLen();

        ssize_t sent = ::send(fd, ptr, len, MSG_NOSIGNAL);

        if (sent < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                m_tcpEpoll->mod(fd, EPOLLIN | EPOLLOUT | EPOLLRDHUP);
                return;
            }
            else
            {
                LOG_ERROR("send error fd={} errno={}", fd, errno);
                closeConnection(fd);
                return;
            }
        }

        txRing.consume(static_cast<size_t>(sent));

        if (static_cast<size_t>(sent) < len)
        {
            m_tcpEpoll->mod(fd, EPOLLIN | EPOLLOUT | EPOLLRDHUP);
            return;
        }
    }

    m_tcpEpoll->mod(fd, EPOLLIN | EPOLLRDHUP);
}
