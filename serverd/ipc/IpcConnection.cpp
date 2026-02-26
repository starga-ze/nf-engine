#include "ipc/IpcConnection.h"

#include <unistd.h>

IpcConnection::IpcConnection(int fd,
                             size_t rxCapacity,
                             size_t txCapacity)
    : m_fd(fd),
      m_rxRing(rxCapacity),
      m_txRing(txCapacity)
{
}

IpcConnection::~IpcConnection()
{
}
