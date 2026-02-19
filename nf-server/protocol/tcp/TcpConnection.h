#pragma once

#include "algorithm/ByteRingBuffer.h"

#include <netinet/in.h>
#include <vector>
#include <deque>
#include <memory>

class TcpConnection
{
public:
    TcpConnection(int fd, const sockaddr_in& peer, size_t rxCapacity, size_t txCapacity);
    ~TcpConnection() = default;

    int fd() const;
    const sockaddr_in& peer() const;

    ByteRingBuffer& rxRing() { return m_rxRing; }
    ByteRingBuffer& txRing() { return m_txRing; }

private:
    ByteRingBuffer m_rxRing;
    ByteRingBuffer m_txRing;
    
    int m_fd;
    sockaddr_in m_peer;
};

