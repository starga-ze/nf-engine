#pragma once

#include "ingress/RxRouter.h"
#include "util/ThreadManager.h"

#include <memory>

class Packet;
class UdpWorker;
class UdpReactor;

class UdpServer
{
public:
    UdpServer(int port, RxRouter* rxRouter, int worker, ThreadManager* threadManager);
    ~UdpServer();

    void start();
    void stop();

    void enqueueTx(std::unique_ptr<Packet> pkt);

private:
    std::vector<std::unique_ptr<UdpWorker>>  m_udpWorkers;
    std::unique_ptr<UdpReactor> m_udpReactor;
};

