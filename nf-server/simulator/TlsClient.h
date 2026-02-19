#pragma once

#include "simulator/TcpClient.h"

#include <openssl/ssl.h>
#include <atomic>

class TlsClient {
public:
    TlsClient(int id, int port);

    ~TlsClient();

    bool connect();
    bool send(const void* data, size_t len);
    ssize_t recv(void* buffer, size_t len);

    void stop();

private:
    bool initSsl(int fd);

    void logError(const char *msg);

    int m_id;
    int m_port;

    TcpClient m_tcp;

    SSL_CTX *m_ctx;
    SSL *m_ssl;
};
