#pragma once

#include "algorithm/ByteRingBuffer.h"

#include <sys/socket.h>
#include <sys/un.h>

#include <cstddef>

class IpcConnection
{
public:
    IpcConnection(int fd,
                  size_t rxCapacity,
                  size_t txCapacity);

    ~IpcConnection();

    IpcConnection(const IpcConnection&) = delete;
    IpcConnection& operator=(const IpcConnection&) = delete;

    int fd() const { return m_fd; }

    ByteRingBuffer& rxRing() { return m_rxRing; }
    ByteRingBuffer& txRing() { return m_txRing; }

private:
    int m_fd{-1};

    ByteRingBuffer m_rxRing;
    ByteRingBuffer m_txRing;
};
