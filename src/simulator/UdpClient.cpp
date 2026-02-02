#include "UdpClient.h"
#include "util/Logger.h"

#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>

UdpClient::UdpClient(int id, int serverPort)
    : m_id(id),
      m_serverPort(serverPort)
{
}

UdpClient::~UdpClient()
{
    stop();
}

bool UdpClient::open()
{
    if (m_sock >= 0)
        return true;

    if (!createSocket())
    {
        LOG_ERROR("UdpClient[{}]: socket create failed", m_id);
        return false;
    }

    if (!setupServerAddr())
    {
        LOG_ERROR("UdpClient[{}]: setup server addr failed", m_id);
        stop();
        return false;
    }

    if (!setNonBlocking())
    {
        LOG_ERROR("UdpClient[{}]: set nonblocking failed", m_id);
        stop();
        return false;
    }

    LOG_INFO("UdpClient[{}]: opened fd={}", m_id, m_sock);
    return true;
}

bool UdpClient::createSocket()
{
    m_sock = ::socket(AF_INET, SOCK_DGRAM, 0);
    return m_sock >= 0;
}

bool UdpClient::setupServerAddr()
{
    std::memset(&m_serverAddr, 0, sizeof(m_serverAddr));
    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_port   = htons(m_serverPort);

    if (::inet_pton(AF_INET, "127.0.0.1", &m_serverAddr.sin_addr) <= 0)
        return false;

    return true;
}

bool UdpClient::setNonBlocking()
{
    int flags = fcntl(m_sock, F_GETFL, 0);
    if (flags < 0)
        return false;

    return fcntl(m_sock, F_SETFL, flags | O_NONBLOCK) == 0;
}

bool UdpClient::send(const void* data, size_t len)
{
    ssize_t n = ::sendto(
        m_sock,
        data,
        len,
        0,
        reinterpret_cast<const sockaddr*>(&m_serverAddr),
        sizeof(m_serverAddr)
    );

    if (n < 0)
    {
        LOG_ERROR("UdpClient[{}]: sendto failed errno={}", m_id, errno);
        return false;
    }

    return true;
}

ssize_t UdpClient::recv(void* buffer, size_t len, int timeoutMs)
{
    pollfd pfd{};
    pfd.fd = m_sock;
    pfd.events = POLLIN;

    int ret = ::poll(&pfd, 1, timeoutMs);
    if (ret <= 0)
    {
        // timeout or error
        return 0;
    }

    ssize_t n = ::recvfrom(m_sock, buffer, len, 0, nullptr, nullptr);
    if (n < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return 0;
        return -1;
    }

    return n;
}

void UdpClient::stop()
{
    if (m_sock < 0)
        return;

    ::close(m_sock);
    m_sock = -1;
}

int UdpClient::getFd() const
{
    return m_sock;
}

