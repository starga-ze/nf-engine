#pragma once

#include "ingress/RxRouter.h"
#include "util/ThreadManager.h"
#include "protocol/tls/TlsServer.h"

#include <memory>

class Packet;
class TcpWorker;
class TcpReactor;

class TcpServer 
{
public:
    TcpServer(int port, RxRouter* rxRouter, int workerCount, 
            ThreadManager* threadManager, std::shared_ptr<TlsServer> tlsServer);

    ~TcpServer();

    void start();
    void stop();

    void enqueueTx(std::unique_ptr<Packet> pkt);

private:
    std::unique_ptr<TcpWorker> m_tcpWorker;
    std::unique_ptr<TcpReactor> m_tcpReactor;
};
