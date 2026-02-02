#pragma once

#include <netinet/in.h>

class TcpConnection
{
public:
    TcpConnection(int fd, const sockaddr_in& peer)
        : m_fd(fd), m_peer(peer)
    {}

    ~TcpConnection() = default;

    int fd() const { return m_fd; }
    const sockaddr_in& peer() const { return m_peer; }
    std::vector<uint8_t>& rxBuffer() { return m_rxBuffer; }


private:
    std::vector<uint8_t> m_rxBuffer;

    int m_fd;
    sockaddr_in m_peer;
};

