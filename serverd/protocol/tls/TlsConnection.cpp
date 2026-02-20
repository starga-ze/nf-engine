#include "protocol/tls/TlsConnection.h"

TlsConnection::TlsConnection(int fd,
                             SSL* ssl,
                             const std::pair<sockaddr_in, sockaddr_in>& addr,
                             size_t rxCapacity) :
    m_fd(fd),
    m_ssl(ssl),
    m_addr(addr),
    m_rxRing(rxCapacity)
{
}

TlsConnection::~TlsConnection()
{
    if (m_ssl)
    {
        SSL_free(m_ssl);
        m_ssl = nullptr;
    }
}

int TlsConnection::fd() const
{
    return m_fd;
}

SSL* TlsConnection::ssl() const
{
    return m_ssl;
}

const sockaddr_in& TlsConnection::serverAddr() const
{
    return m_addr.first;
}

const sockaddr_in& TlsConnection::peerAddr() const
{
    return m_addr.second;
}

ByteRingBuffer& TlsConnection::rxRing()
{
    return m_rxRing;
}

std::deque<std::unique_ptr<Packet>>& TlsConnection::txQ()
{
    return m_txQueue;
}
