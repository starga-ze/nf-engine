#pragma once

#include "protocol/tcp/TcpWorker.h"
#include "protocol/tcp/TcpEpoll.h"

class TlsServer;


class TcpReactor
{
public:
    TcpReactor(int port, TcpWorker* tcpWorker, std::shared_ptr<TlsServer> tlsServer);
    ~TcpReactor();

    void start();
    void stop();

private:
    bool init();
    bool create();
    bool setSockOpt();
    bool bindAndListen();
    void close();
    bool setNonBlocking(int fd);

    TcpWorker* m_tcpWorker;
    std::shared_ptr<TlsServer> m_tlsServer;
    std::unique_ptr<TcpEpoll> m_tcpEpoll;

    int m_port;
    int m_listenFd;
    sockaddr_in m_serverAddr{};
    std::atomic<bool> m_running = false;
};
