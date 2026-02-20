#pragma once

#include <memory>
#include <atomic>
#include <vector>

#include "packet/Packet.h"

class UdpWorker;
class Epoll;
class MpscQueue;

class UdpReactor
{
public:
    UdpReactor(int port, std::vector<std::unique_ptr<UdpWorker>>& udpWorkers);
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

    void receivePackets();
    void processTxQueue();
    void flushPacket(std::unique_ptr<Packet>& pkt);

    void shutdown();

private:
    int m_port;
    int m_sockFd{-1};

    sockaddr_in m_serverAddr{};
    std::vector<uint8_t> m_rxBuffer;

    std::vector<UdpWorker*> m_udpWorkers;

    std::atomic<bool> m_running{false};

    std::unique_ptr<Epoll> m_udpEpoll;

    std::unique_ptr<MpscQueue> m_txQueue;
};
