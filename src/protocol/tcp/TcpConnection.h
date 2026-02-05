#pragma once

#include "algorithm/RingBuffer.h"

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
    TcpConnection(int fd, const sockaddr_in& peer, size_t rxMaxBufferSize);
    ~TcpConnection() = default;

    int fd() const;
    const sockaddr_in& peer() const;
    std::deque<TxBuffer>& txQueue();
    RingBuffer& rxRing() { return m_rxRing; }

private:
    RingBuffer m_rxRing;

    std::deque<TxBuffer> m_txQueue;
    
    int m_fd;
    sockaddr_in m_peer;
};

