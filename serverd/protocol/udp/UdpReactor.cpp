#include "UdpReactor.h"

#include "protocol/udp/UdpWorker.h"
#include "util/Logger.h"
#include "io/Epoll.h"
#include "algorithm/MpscQueue.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>

#define UDP_RECV_CHUNK_SIZE         (4096)
#define UDP_MAX_EVENTS              (64)
#define UDP_MAX_RX_BUFFER_SIZE      (65536) // 64KB
#define UDP_MPSC_QUEUE_SIZE         (65536)

UdpReactor::UdpReactor(int port, std::vector<std::unique_ptr<UdpWorker>>& udpWorkers)
    : m_port(port)
{
    m_udpWorkers.reserve(udpWorkers.size());

    if (udpWorkers.empty())
    {
        LOG_FATAL("UdpReactor requires at at leat one UdpWorker");
        std::abort();
    }

    for (const auto& worker : udpWorkers)
    {
        m_udpWorkers.push_back(worker.get());
    }

    m_txQueue = std::make_unique<MpscQueue>(UDP_MPSC_QUEUE_SIZE);
    m_udpEpoll = std::make_unique<Epoll>();
}

UdpReactor::~UdpReactor()
{
    stop();
    shutdown();
}

void UdpReactor::start()
{
    if (!init())
        return;

    if (!m_udpEpoll->init())
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

            if (fd == m_udpEpoll->getWakeupFd())
            {
                m_udpEpoll->drainWakeup();
                processTxQueue();
                continue;
            }

            if (fd == m_sockFd)
            {
                receivePackets();
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
    if (!createSocket()) return false;
    if (!setSockOpt())   return false;
    if (!bindSocket())   return false;

    LOG_INFO("UDP listen ready port={}", m_port);
    return true;
}

bool UdpReactor::createSocket()
{
    m_sockFd = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (m_sockFd < 0)
    {
        LOG_ERROR("UDP socket create failed errno={}", errno);
        return false;
    }

    return setNonBlocking(m_sockFd);
}

bool UdpReactor::setSockOpt()
{
    int opt = 1;
    ::setsockopt(m_sockFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    int rcv = UDP_MAX_RX_BUFFER_SIZE;
    ::setsockopt(m_sockFd, SOL_SOCKET, SO_RCVBUF, &rcv, sizeof(rcv));

    return true;
}

bool UdpReactor::bindSocket()
{
    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_addr.s_addr = INADDR_ANY;
    m_serverAddr.sin_port = htons(m_port);

    if (::bind(m_sockFd,
               reinterpret_cast<sockaddr*>(&m_serverAddr),
               sizeof(m_serverAddr)) != 0)
    {
        LOG_ERROR("UDP bind failed errno={}", errno);
        return false;
    }

    return true;
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
            std::vector<uint8_t> payload(
                m_rxBuffer.begin(),
                m_rxBuffer.begin() + bytes
            );

            auto pkt = std::make_unique<Packet>(
                m_sockFd,
                Protocol::UDP,
                std::move(payload),
                clientAddr,
                m_serverAddr
            );

            uint32_t key = ntohl(clientAddr.sin_addr.s_addr) ^ ntohs(clientAddr.sin_port);
            size_t idx = key % m_udpWorkers.size();

            m_udpWorkers[idx]->enqueueRx(std::move(pkt));
            continue;
        }

        if (bytes == 0)
            return;

        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return;

        if (errno == EINTR)
            continue;

        LOG_ERROR("UDP recvfrom failed errno={}", errno);
        return;
    }
}

void UdpReactor::enqueueTx(std::unique_ptr<Packet> pkt)
{
    if (!pkt) return;

    if (m_txQueue->enqueue(std::move(pkt)))
    {
        m_udpEpoll->wakeup();
    }
    else
    {
        LOG_WARN("UDP Tx MPSC queue full, packet dropped");
    }
}

void UdpReactor::processTxQueue()
{
    std::vector<std::unique_ptr<Packet>> packets;

    m_txQueue->dequeueAll(packets);

    for (auto& pkt : packets)
    {
        flushPacket(pkt);
    }
}

void UdpReactor::flushPacket(std::unique_ptr<Packet>& pkt)
{
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

    if (ret < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            m_txQueue->enqueue(std::move(pkt));
            m_udpEpoll->wakeup();
            return;
        }

        if (errno == EINTR)
        {
            m_txQueue->enqueue(std::move(pkt));
            return;
        }

        LOG_ERROR("UDP sendto failed errno={}", errno);
    }
}

void UdpReactor::shutdown()
{
    if (m_udpEpoll)
        m_udpEpoll->close();

    if (m_sockFd >= 0)
    {
        ::close(m_sockFd);
        m_sockFd = -1;
    }
}
