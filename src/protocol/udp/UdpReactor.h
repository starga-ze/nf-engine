#pragma once

#include "packet/Packet.h"

#include <atomic>
#include <deque>
#include <mutex>
#include <vector>
#include <memory>
#include <netinet/in.h>

class UdpWorker;
class UdpEpoll;

class UdpReactor
{
public:
    UdpReactor(int port, UdpWorker* udpWorker);
    ~UdpReactor();

    void start();
    void stop();

    void enqueueTx(std::unique_ptr<Packet> pkt);

private:
    bool init();
    bool createSocket();
    bool setSockOpt();
    bool bindSocket();
    bool setNonBlocking(int fd);
    void shutdown();

    void receivePackets();
    void snapshotPendingTx();
    void flushAllPending(size_t budgetItems);
    size_t flushPending(size_t budgetItems);

private:
    int m_port{0};
    UdpWorker* m_udpWorker{nullptr};

    std::unique_ptr<UdpEpoll> m_udpEpoll;

    int m_sockFd{-1};
    sockaddr_in m_serverAddr{};

    std::atomic<bool> m_running{false};

    std::vector<uint8_t> m_rxBuffer;

    std::mutex m_pendingTxLock;
    std::deque<std::unique_ptr<Packet>> m_pendingTx;
    std::deque<std::unique_ptr<Packet>> m_snapshotTx;
};

