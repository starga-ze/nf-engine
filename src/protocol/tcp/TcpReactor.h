#pragma once

#include "protocol/tcp/TcpWorker.h"

class TlsServer;


class TcpReactor
{
public:
    TcpReactor(int port, TcpWorker* tcpWorker, std::shared_ptr<TlsServer> tlsServer);
    ~TcpReactor();

    void start();
    void stop();
};
