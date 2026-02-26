#pragma once

#include <atomic>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>

#include "io/Epoll.h"
#include "algorithm/ByteRingBuffer.h"
#include "algorithm/MpscQueue.h"

class IpcWorker;
class IpcMessage;

class IpcReactor
{
public:
    IpcReactor(const std::string& socketPath,
               std::vector<std::unique_ptr<IpcWorker>>& ipcWorkers);
    ~IpcReactor();

    void start();
    void stop();

    void enqueueTx(std::unique_ptr<IpcMessage> msg);

private:
    struct Conn
    {
        explicit Conn(size_t rxCap, size_t txCap)
            : rxRing(rxCap),
              txRing(txCap)
        {}
        ByteRingBuffer rxRing;
        ByteRingBuffer txRing;
    };

    bool init();
    bool createSocket();
    bool bindAndListen();
    bool setNonBlocking(int fd);

    void shutdown();
    void closeListen();

    void acceptConnection();
    void closeConnection(int fd);

    void receive(int fd);

    void processTxQueue();
    void flushTxBuffer(int fd);

private:
    std::string m_socketPath;

    int m_listenFd{-1};

    std::unique_ptr<Epoll> m_epoll;
    std::unique_ptr<MpscQueue<IpcMessage>> m_txQueue;

    std::vector<IpcWorker*> m_ipcWorkers; // non-owning
    std::unordered_map<int, std::unique_ptr<Conn>> m_conns;

    std::atomic<bool> m_running{false};
    alignas(64) std::atomic<size_t> m_rr{0};
};
