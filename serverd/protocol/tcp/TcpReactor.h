#pragma once

#include "protocol/tcp/TcpWorker.h"
#include "protocol/tcp/TcpConnection.h"
#include "protocol/tcp/TcpFraming.h"

#include "algorithm/ByteRingBuffer.h"
#include "algorithm/MpscQueue.h"

#include "packet/Packet.h"

#include "io/Epoll.h"

#include <unordered_map>
#include <unordered_set>

class TlsServer;

class TcpReactor
{
public:
    TcpReactor(int port, std::vector<std::unique_ptr<TcpWorker>>& tcpWorkers, 
            std::shared_ptr<TlsServer> tlsServer);
    ~TcpReactor();

    void start();
    void stop();

    void enqueueTx(std::unique_ptr<Packet> pkt);

private:
    bool init();
    bool create();
    bool setSockOpt();
    bool bindAndListen();
    void close();
    bool setNonBlocking(int fd);
    void acceptConnection();
    void closeConnection(int fd);
    void receive(int fd);
    bool isTlsClientHello(int fd);
    void handoverToTls(int fd);
    void shutdown();

    void processTxQueue();
    void flushTxBuffer(int fd);

    std::vector<TcpWorker*> m_tcpWorkers;
    std::shared_ptr<TlsServer> m_tlsServer;
    std::unique_ptr<Epoll> m_tcpEpoll;

    std::unordered_map<int, std::unique_ptr<TcpConnection>> m_conns;

    std::unique_ptr<MpscQueue<Packet>> m_txQueue;

    int m_port;
    int m_listenFd;
    sockaddr_in m_serverAddr{};
    std::atomic<bool> m_running = false;

};
