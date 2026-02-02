#include "UdpReactor.h"

#include "protocol/udp/UdpEpoll.h"
#include "protocol/udp/UdpWorker.h"

#include "util/Logger.h"
#include "packet/Packet.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>

#define UDP_RECV_CHUNK_SIZE     (2048)
#define UDP_MAX_RX_BUFFER_SIZE  (256 * 1024)
#define UDP_MAX_EVENTS          (64)

UdpReactor::UdpReactor(int port, UdpWorker* udpWorker)
    : m_port(port),
      m_udpWorker(udpWorker)
{
    m_udpEpoll = std::make_unique<UdpEpoll>();
}

UdpReactor::~UdpReactor()
{
    stop();
    shutdown();
}

void UdpReactor::start()
{
    if (not init())
        return;

    if (not m_udpEpoll->init())
    {
        shutdown();
        return;
    }

    m_udpEpoll->add(m_sockFd, EPOLLIN);

    m_running = true;

    std::vector<epoll_event> events(UDP_MAX_EVENTS);
    m_rxBuffer.resize(UDP_RECV_CHUNK_SIZE);

    while (m_running)
    {
        int n = m_udpEpoll->wait(events, -1);
        if (n < 0)
        {
            if (errno == EINTR) continue;
            continue;
        }

        for (int i = 0; i < n; ++i)
        {
            int fd = events[i].data.fd;
            uint32_t ev = events[i].events;

            if (fd == m_udpEpoll->getWakeupFd())
            {
                m_udpEpoll->drainWakeup();
                snapshotPendingTx();
                flushAllPending(256);
                continue;
            }

            if (fd == m_sockFd)
            {
                if (ev & (EPOLLERR | EPOLLHUP))
                {
                    LOG_ERROR("UdpReactor: socket error/hup");
                    m_running = false;
                    break;
                }

                if (ev & EPOLLIN)
                {
                    receivePackets();
                }
            }
        }
    }

    shutdown();
}

void UdpReactor::stop()
{
    m_running = false;
    if (m_udpEpoll) m_udpEpoll->wakeup();
}

bool UdpReactor::init()
{
    if (not createSocket()) return false;
    if (not setSockOpt())   return false;
    if (not bindSocket())   return false;

    LOG_INFO("UDP listen ready port={}", m_port);
    return true;
}

bool UdpReactor::createSocket()
{
    m_sockFd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (m_sockFd < 0)
    {
        LOG_ERROR("UdpReactor: socket create failed errno={}", errno);
        return false;
    }

    if (not setNonBlocking(m_sockFd))
    {
        LOG_ERROR("UdpReactor: setNonBlocking failed errno={}", errno);
        return false;
    }

    return true;
}

bool UdpReactor::setSockOpt()
{
    int opt = 1;
    (void)::setsockopt(m_sockFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    int rcv = UDP_MAX_RX_BUFFER_SIZE;
    (void)::setsockopt(m_sockFd, SOL_SOCKET, SO_RCVBUF, &rcv, sizeof(rcv));

    return true;
}

bool UdpReactor::bindSocket()
{
    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_addr.s_addr = INADDR_ANY;
    m_serverAddr.sin_port = htons(m_port);

    if (::bind(m_sockFd, reinterpret_cast<sockaddr*>(&m_serverAddr), sizeof(m_serverAddr)) != 0)
    {
        LOG_ERROR("UdpReactor: bind failed errno={}", errno);
        return false;
    }

    return true;
}

void UdpReactor::shutdown()
{
    {
        std::lock_guard<std::mutex> lock(m_pendingTxLock);
        m_pendingTx.clear();
        m_snapshotTx.clear();
    }

    if (m_udpEpoll)
        m_udpEpoll->close();

    if (m_sockFd >= 0)
    {
        ::close(m_sockFd);
        m_sockFd = -1;
    }
}

bool UdpReactor::setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

void UdpReactor::receivePackets()
{
    while (true)
    {
        sockaddr_in clientAddr{};
        socklen_t addrLen = sizeof(clientAddr);

        ssize_t bytes = ::recvfrom(
            m_sockFd,
            m_rxBuffer.data(),
            m_rxBuffer.size(),
            0,
            reinterpret_cast<sockaddr*>(&clientAddr),
            &addrLen
        );

        if (bytes > 0)
        {
            std::vector<uint8_t> payload(m_rxBuffer.begin(), m_rxBuffer.begin() + bytes);

            LOG_TRACE("recv bytes ={}", bytes);

            auto pkt = std::make_unique<Packet>(
                m_sockFd,
                Protocol::UDP,
                std::move(payload),
                clientAddr,
                m_serverAddr
            );

            m_udpWorker->enqueueRx(std::move(pkt));
            continue;
        }

        if (bytes == 0)
            return;

        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;

        if (errno == EINTR)
            continue;

        LOG_ERROR("UdpReactor: recvfrom failed errno={}", errno);
        return;
    }
}

void UdpReactor::enqueueTx(std::unique_ptr<Packet> pkt)
{
    if (!pkt) return;

    {
        std::lock_guard<std::mutex> lock(m_pendingTxLock);
        m_pendingTx.push_back(std::move(pkt));
    }

    if (m_udpEpoll) m_udpEpoll->wakeup();
}

void UdpReactor::snapshotPendingTx()
{
    std::lock_guard<std::mutex> lock(m_pendingTxLock);
    while (!m_pendingTx.empty())
    {
        m_snapshotTx.push_back(std::move(m_pendingTx.front()));
        m_pendingTx.pop_front();
    }
}

void UdpReactor::flushAllPending(size_t budgetItems)
{
    (void)flushPending(budgetItems);
}

size_t UdpReactor::flushPending(size_t budgetItems)
{
    size_t used = 0;

    while (used < budgetItems)
    {
        if (m_snapshotTx.empty())
            return used;

        auto pkt = std::move(m_snapshotTx.front());
        m_snapshotTx.pop_front();

        const auto& payload = pkt->getPayload();

        sockaddr_in dstAddr{};
        dstAddr.sin_family = AF_INET;
        dstAddr.sin_addr.s_addr = htonl(pkt->getDstIp());
        dstAddr.sin_port = htons(pkt->getDstPort());

        ssize_t ret = ::sendto(
            m_sockFd,
            payload.data(),
            payload.size(),
            0,
            reinterpret_cast<const sockaddr*>(&dstAddr),
            sizeof(dstAddr)
        );

        if (ret >= 0)
        {
            used++;
            continue;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            // 못 보낸 건 우선순위 유지 위해 snapshot 앞쪽으로 복귀
            m_snapshotTx.push_front(std::move(pkt));
            if (m_udpEpoll) m_udpEpoll->wakeup();
            return used + 1;
        }

        if (errno == EINTR)
        {
            m_snapshotTx.push_front(std::move(pkt));
            continue;
        }

        LOG_ERROR("UdpReactor: sendto failed errno={}", errno);
        used++;
    }

    // budget 초과했는데 남아있으면 다음 wakeup에서 계속 flush
    if (!m_snapshotTx.empty())
    {
        if (m_udpEpoll) m_udpEpoll->wakeup();
    }

    return used;
}

