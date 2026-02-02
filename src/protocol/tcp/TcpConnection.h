#pragma once

#include <netinet/in.h>
#include <vector>
#include <deque>
#include <memory>

struct TxBuffer
{
    std::vector<uint8_t> data;
    size_t offset{0};
};

class TcpConnection
{
public:
    TcpConnection(int fd, const sockaddr_in& peer);
    ~TcpConnection() = default;

    int fd() const;
    const sockaddr_in& peer() const;
    std::vector<uint8_t>& rxBuffer();

    std::deque<TxBuffer>& pendingTxQueue();
    std::deque<TxBuffer>& txQueue();

private:
    std::vector<uint8_t> m_rxBuffer;

    std::deque<TxBuffer> m_pendingTxQueue;
    std::deque<TxBuffer> m_txQueue;

    int m_fd;
    sockaddr_in m_peer;
};

