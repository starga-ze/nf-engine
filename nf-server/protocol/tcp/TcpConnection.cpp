#include "protocol/tcp/TcpConnection.h"

TcpConnection::TcpConnection(int fd, const sockaddr_in& peer, 
        size_t rxCapacity, size_t txCapacity) :
    m_fd(fd),
    m_peer(peer),
    m_rxRing(rxCapacity),
    m_txRing(txCapacity)
{

}

int TcpConnection::fd() const
{
    return m_fd;
}

const sockaddr_in& TcpConnection::peer() const
{
    return m_peer;
}

