#include "protocol/tcp/TcpConnection.h"

TcpConnection::TcpConnection(int fd, const sockaddr_in& peer) :
    m_fd(fd),
    m_peer(peer)
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

std::vector<uint8_t>& TcpConnection::rxBuffer()
{
    return m_rxBuffer;
}

std::deque<TxBuffer>& TcpConnection::pendingTxQueue()
{
    return m_pendingTxQueue;
}

std::deque<TxBuffer>& TcpConnection::txQueue()
{
    return m_txQueue;
}
