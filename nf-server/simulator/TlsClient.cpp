#include "TlsClient.h"
#include "util/Logger.h"
#include "util/Random.h"

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <thread>
#include <chrono>
#include <cstring>
#include <fcntl.h>

TlsClient::TlsClient(int id, int port)
        : m_id(id),
          m_port(port),
          m_tcp(id, port),
          m_ctx(nullptr),
          m_ssl(nullptr)
{

}

TlsClient::~TlsClient() {
    stop();
}

void TlsClient::logError(const char *msg) {
    unsigned long e = ERR_get_error();
    if (e == 0) {
        LOG_ERROR("TlsClient[{}]: {} (no detail)", m_id, msg);
        return;
    }

    char buf[256];
    ERR_error_string_n(e, buf, sizeof(buf));
    LOG_ERROR("TlsClient[{}]: {}: {}", m_id, msg, buf);
}

bool TlsClient::initSsl(int fd) {
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    m_ctx = SSL_CTX_new(TLS_client_method());
    if (!m_ctx) {
        logError("SSL_CTX_new failed");
        return false;
    }

    SSL_CTX_set_verify(m_ctx, SSL_VERIFY_NONE, nullptr);

    m_ssl = SSL_new(m_ctx);
    if (!m_ssl) {
        logError("SSL_new failed");
        return false;
    }

    if (SSL_set_fd(m_ssl, fd) != 1) {
        logError("SSL_set_fd failed");
        return false;
    }

    return true;
}

bool TlsClient::connect()
{
    if (!m_tcp.connect()) {
        LOG_ERROR("TlsClient[{}]: TCP connect failed", m_id);
        return false;
    }

    int fd = m_tcp.getFd();
    LOG_INFO("TlsClient[{}]: TCP connected, fd={}", m_id, fd);

    if (!initSsl(fd)) {
        LOG_ERROR("TlsClient[{}]: initSsl failed", m_id);
        stop();
        return false;
    }

    while (true) {
        int ret = SSL_connect(m_ssl);
        if (ret == 1)
            break;

        int err = SSL_get_error(m_ssl, ret);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        LOG_ERROR("TlsClient[{}]: SSL_connect failed (ssl_err={})",
                  m_id, err);
        stop();
        return false;
    }

    LOG_INFO("TlsClient[{}]: TLS Handshake complete!", m_id);
    return true;
}


bool TlsClient::send(const void* data, size_t len)
{
    int n = SSL_write(m_ssl, data, len);
    if (n <= 0) {
        logError("SSL_write failed");
        return false;
    }
    return true;
}

ssize_t TlsClient::recv(void* buffer, size_t len)
{
    int n = SSL_read(m_ssl, buffer, len);
    if (n <= 0) {
        int err = SSL_get_error(m_ssl, n);
        if (err == SSL_ERROR_WANT_READ)
            return 0;
        return -1;
    }
    return n;
}

void TlsClient::stop() {
    if (!m_ssl && !m_ctx)
        return;

    if (m_ssl) {
        SSL_shutdown(m_ssl);
        SSL_free(m_ssl);
        m_ssl = nullptr;
    }

    if (m_ctx) {
        SSL_CTX_free(m_ctx);
        m_ctx = nullptr;
    }

    m_tcp.stop();
}


