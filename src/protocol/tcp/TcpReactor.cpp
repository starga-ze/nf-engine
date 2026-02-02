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
#define TCP_MAX_RX_BUFFER_SIZE  (256 * 1024);

TcpReactor::TcpReactor(int port, TcpWorker* tcpWorker, std::shared_ptr<TlsServer> tlsServer) :
    m_port(port),
    m_tcpWorker(tcpWorker),
    m_tlsServer(tlsServer)
{
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
    m_rxBuffer.resize(TCP_RECV_CHUNK_SIZE);

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
                snapshotPendingTx();
                flushAllTxQueue(256);
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
                flushTxQueueForFd(fd, 128);
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

        int rcv = TCP_MAX_RX_BUFFER_SIZE;
        ::setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcv, sizeof(rcv));

        auto conn = std::make_unique<TcpConnection>(fd, peer);

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
    auto& rxBuffer = conn->rxBuffer();

    while (true) {
        ssize_t bytes = ::recv(fd, m_rxBuffer.data(), m_rxBuffer.size(), 0);

        if (bytes > 0) {
            rxBuffer.insert(rxBuffer.end(), m_rxBuffer.begin(), m_rxBuffer.begin() + bytes);

            while (true) {
                std::vector<uint8_t> payload;
                uint16_t bodyLen = 0;

                TcpFramingResult r = TcpFraming::tryExtractFrame(rxBuffer, payload, bodyLen);

                if (r == TcpFramingResult::Ok) {
                    auto pkt = std::make_unique<Packet>(fd, Protocol::TCP, std::move(payload), 
                            conn->peer(), m_serverAddr);

                    m_tcpWorker->enqueueRx(std::move(pkt));
                    continue;
                }

                if (r == TcpFramingResult::NeedMoreData) {
                    break; 
                }

                closeConnection(fd);
                return;
            }
        }
        else {
            if (bytes == 0) 
            {
                closeConnection(fd);
                return;
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
    if (!pkt) return;

    TxRequest req;
    req.fd = pkt->getFd();
    req.buf.data = pkt->takePayload();
    req.buf.offset = 0;

    {
        std::lock_guard<std::mutex> lock(m_pendingTxLock);
        m_pendingTx.push_back(std::move(req));
    }

    m_tcpEpoll->wakeup();
}

void TcpReactor::snapshotPendingTx()
{
    std::deque<TxRequest> local;
    {
        std::lock_guard<std::mutex> lock(m_pendingTxLock);
        local.swap(m_pendingTx);
    }

    for (auto& req : local)
    {
        auto it = m_conns.find(req.fd);
        if (it == m_conns.end())
        {
            continue;
        }

        auto& conn = it->second;

        conn->txQueue().push_back(std::move(req.buf));

        m_tcpEpoll->mod(req.fd, EPOLLIN | EPOLLOUT | EPOLLRDHUP);
    }
}


void TcpReactor::flushAllTxQueue(size_t budgetPkts)
{
    size_t used = 0;

    for (auto& [fd, conn] : m_conns)
    {
        if (used >= budgetPkts)
            break;

        if (conn->txQueue().empty())
            continue;

        used += flushTxQueueForFd(fd, budgetPkts - used);
    }
}

size_t TcpReactor::flushTxQueueForFd(int fd, size_t budgetPkts)
{
    auto it = m_conns.find(fd);
    if (it == m_conns.end())
        return 0;

    auto& q = it->second->txQueue();
    size_t used = 0;

    while (used < budgetPkts && !q.empty())
    {
        auto& buf = q.front();

        while (buf.offset < buf.data.size())
        {
            const uint8_t* p = buf.data.data() + buf.offset;
            size_t remain = buf.data.size() - buf.offset;

            ssize_t n = ::send(fd, p, remain, MSG_NOSIGNAL);
            if (n < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                {
                    m_tcpEpoll->mod(fd, EPOLLIN | EPOLLOUT | EPOLLRDHUP);
                    return used + 1;
                }

                closeConnection(fd);
                return used + 1;
            }

            buf.offset += static_cast<size_t>(n);
        }

        q.pop_front();
        used++;
    }

    if (q.empty())
        m_tcpEpoll->mod(fd, EPOLLIN | EPOLLRDHUP);
    else
        m_tcpEpoll->mod(fd, EPOLLIN | EPOLLOUT | EPOLLRDHUP);

    return used;
}
