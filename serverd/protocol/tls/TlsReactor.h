#pragma once

#include "protocol/tls/TlsWorker.h"

#include "algorithm/LockQueue.h"
#include "algorithm/MpscQueue.h"
#include "io/Epoll.h"

#include <openssl/ssl.h>

#include <unordered_map>
#include <vector>
#include <memory>
#include <atomic>
#include <utility>
#include <netinet/in.h>

class Packet;
class TlsConnection;

struct TlsHandoverItem
{
    int fd;
    std::pair<sockaddr_in, sockaddr_in> connInfo;
};

class TlsReactor
{
public:
    TlsReactor(SSL_CTX* ctx,
               std::vector<std::unique_ptr<TlsWorker>>& tlsWorkers);

    ~TlsReactor();

    void start();
    void stop();

    void enqueueTx(std::unique_ptr<Packet> pkt);
    void handover(int fd, std::pair<sockaddr_in, sockaddr_in> connInfo);

private:
    bool setNonBlocking(int fd);

    void shutdown();
    void closeConnection(int fd);

    void processWakeup();
    void processHandover();
    void processTxQueue();

    void onReadable(int fd);
    void onWritable(int fd);

    bool isHandshakeDone(int fd) const;
    bool driveHandshake(int fd);

    void receive(int fd);
    void flushTx(int fd, size_t budget);
    void updateInterest(int fd);

    SSL_CTX* m_ctx;
    std::vector<TlsWorker*> m_tlsWorkers;

    std::unique_ptr<Epoll> m_epoll;
    std::unique_ptr<MpscQueue<Packet>> m_txQueue;

    std::unordered_map<int, std::unique_ptr<TlsConnection>> m_conns;

    LockQueue<TlsHandoverItem> m_handoverQueue;

    std::atomic<bool> m_running{false};
};
