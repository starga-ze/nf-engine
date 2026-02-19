#pragma once

#include <cstddef>
#include <cstdint>
#include <netinet/in.h>

class UdpClient
{
public:
    UdpClient(int id, int serverPort);
    ~UdpClient();

    bool open();
    bool send(const void* data, size_t len);
    ssize_t recv(void* buffer, size_t len, int timeoutMs);
    void stop();

    int getFd() const;

private:
    bool createSocket();
    bool setupServerAddr();
    bool setNonBlocking();

private:
    int m_id{0};
    int m_serverPort{0};

    int m_sock{-1};
    sockaddr_in m_serverAddr{};
};

