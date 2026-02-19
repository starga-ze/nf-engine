#pragma once

#include <cstddef>
#include <cstdint>
#include <netinet/in.h>

class TcpClient {
public:
    TcpClient(int id, int serverPort);
    ~TcpClient();

    bool connect();
    bool send(const void* data, size_t len);
    ssize_t recv(void* buffer, size_t len);
    void stop();

    int getFd() const;

private:
    bool createSocket();
    bool connectServer();

private:
    int m_id;
    int m_serverPort;
    int m_sock;
    sockaddr_in m_serverAddr;
};

