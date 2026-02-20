#pragma once

#include "ingress/RxRouter.h"
#include "util/ThreadManager.h"

#include <openssl/ssl.h>

#include <memory>
#include <vector>
#include <utility>
#include <netinet/in.h>

class Packet;
class TlsWorker;
class TlsReactor;

class TlsServer
{
public:
    TlsServer(SSL_CTX* ctx, RxRouter* rxRouter, int workerCount, ThreadManager* threadManager);
    ~TlsServer();

    void start();
    void stop();

    void enqueueTx(std::unique_ptr<Packet> pkt);

    void handleTlsConnection(int fd, std::pair<sockaddr_in, sockaddr_in> connInfo);

private:
    std::vector<std::unique_ptr<TlsWorker>> m_tlsWorkers;
    std::unique_ptr<TlsReactor> m_tlsReactor;
};
