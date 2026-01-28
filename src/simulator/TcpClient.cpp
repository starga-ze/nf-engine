#include "TcpClient.h"
#include "util/Logger.h"

#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>

TcpClient::TcpClient(int id, int serverPort)
    : m_id(id),
      m_serverPort(serverPort),
      m_sock(-1)
{

}

TcpClient::~TcpClient() {
    stop();
}

bool TcpClient::connect() {
    if (m_sock >= 0)
        return true;

    if (!createSocket()) {
        LOG_ERROR("TcpClient[{}]: socket create failed", m_id);
        return false;
    }

    if (!connectServer()) {
        LOG_ERROR("TcpClient[{}]: connect failed", m_id);
        stop();
        return false;
    }

    int flags = fcntl(m_sock, F_GETFL, 0);
    if (flags < 0)
        return false;

    if (fcntl(m_sock, F_SETFL, flags | O_NONBLOCK) < 0)
        return false;

    LOG_INFO("TcpClient[{}]: connected fd={}", m_id, m_sock);
    return true;
}

bool TcpClient::createSocket() {
    m_sock = socket(AF_INET, SOCK_STREAM, 0);
    return m_sock >= 0;
}

bool TcpClient::connectServer() {
    std::memset(&m_serverAddr, 0, sizeof(m_serverAddr));
    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_port = htons(m_serverPort);

    if (inet_pton(AF_INET, "127.0.0.1", &m_serverAddr.sin_addr) <= 0)
        return false;

    if (::connect(m_sock,
                  reinterpret_cast<sockaddr*>(&m_serverAddr),
                  sizeof(m_serverAddr)) < 0)
        return false;

    return true;
}

bool TcpClient::send(const void* data, size_t len) {
    ssize_t n = ::send(m_sock, data, len, 0);
    if (n <= 0) {
        LOG_ERROR("TcpClient[{}]: send failed", m_id);
        return false;
    }
    return true;
}

ssize_t TcpClient::recv(void* buffer, size_t len) {
    ssize_t n = ::recv(m_sock, buffer, len, 0);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return 0;
        return -1;
    }
    return n;
}

void TcpClient::stop() {
    if (m_sock < 0)
        return;


    if (m_sock >= 0) {
        close(m_sock);
        m_sock = -1;
    }
}

int TcpClient::getFd() const {
    return m_sock;
}

