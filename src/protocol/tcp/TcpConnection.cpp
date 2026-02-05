#include "protocol/tcp/TcpConnection.h"

TcpConnection::TcpConnection(int fd, const sockaddr_in& peer, size_t rxMaxBufferSize) :
    m_fd(fd),
    m_peer(peer),
    m_rxRing(rxMaxBufferSize)
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

std::deque<TxBuffer>& TcpConnection::txQueue()
{
    return m_txQueue;
}
