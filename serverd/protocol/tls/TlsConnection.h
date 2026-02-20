#pragma once

#include "algorithm/ByteRingBuffer.h"
#include "packet/Packet.h"

#include <openssl/ssl.h>

#include <deque>
#include <memory>
#include <netinet/in.h>
#include <utility>

class TlsConnection
{
public:
    TlsConnection(int fd,
                  SSL* ssl,
                  const std::pair<sockaddr_in, sockaddr_in>& addr,
                  size_t rxCapacity);

    ~TlsConnection();

    int fd() const;
    SSL* ssl() const;

    const sockaddr_in& serverAddr() const;
    const sockaddr_in& peerAddr() const;

    ByteRingBuffer& rxRing();
    std::deque<std::unique_ptr<Packet>>& txQ();

private:
    int m_fd;
    SSL* m_ssl;
    std::pair<sockaddr_in, sockaddr_in> m_addr;

    ByteRingBuffer m_rxRing;
    std::deque<std::unique_ptr<Packet>> m_txQueue;
};
