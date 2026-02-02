#pragma once

#include "protocol/tcp/TcpWorker.h"
#include "protocol/tcp/TcpEpoll.h"
#include "protocol/tcp/TcpConnection.h"
#include "protocol/tcp/TcpFraming.h"

#include "packet/Packet.h"

#include <unordered_map>
#include <mutex>

class TlsServer;

struct TxRequest
{
    int fd;
    TxBuffer buf;
};

class TcpReactor
{
public:
    TcpReactor(int port, TcpWorker* tcpWorker, std::shared_ptr<TlsServer> tlsServer);
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

    void snapshotPendingTx();
    void flushAllTxQueue(size_t budget);
    size_t flushTxQueueForFd(int fd, size_t budget);


    TcpWorker* m_tcpWorker;
    std::shared_ptr<TlsServer> m_tlsServer;
    std::unique_ptr<TcpEpoll> m_tcpEpoll;

    std::unordered_map<int, std::unique_ptr<TcpConnection>> m_conns;
    std::vector<uint8_t> m_rxBuffer;

    int m_port;
    int m_listenFd;
    sockaddr_in m_serverAddr{};
    std::atomic<bool> m_running = false;

    std::mutex m_pendingTxLock;
    std::deque<TxRequest> m_pendingTx;
};
