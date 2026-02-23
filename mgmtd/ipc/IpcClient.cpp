#include "ipc/IpcClient.h"
#include "util/Logger.h"
#include "ipc/IpcFraming.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <stdexcept>

static bool writeAll(int fd, const void* data, size_t len)
{
    const uint8_t* p = static_cast<const uint8_t*>(data);
    size_t off = 0;

    while (off < len)
    {
        ssize_t w = ::send(fd, p + off, len - off, MSG_NOSIGNAL);
        if (w < 0)
        {
            if (errno == EINTR)
                continue;
            return false;
        }
        off += static_cast<size_t>(w);
    }
    return true;
}

static bool readExact(int fd, void* out, size_t len)
{
    uint8_t* p = static_cast<uint8_t*>(out);
    size_t off = 0;

    while (off < len)
    {
        ssize_t r = ::read(fd, p + off, len - off);
        if (r < 0)
        {
            if (errno == EINTR)
                continue;
            return false;
        }
        if (r == 0)
            return false;
        off += static_cast<size_t>(r);
    }
    return true;
}

IpcClient::IpcClient(std::string path)
    : m_socketPath(std::move(path))
{
}

std::string IpcClient::send(const std::string& payload)
{
    if (payload.size() > IpcFraming::MAX_BODY)
        throw std::runtime_error("IpcClient: payload too large");

    int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0)
    {
        LOG_ERROR("IpcClient socket() failed: {}", std::strerror(errno));
        throw std::runtime_error("IpcClient: socket() failed");
    }

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;

    if (m_socketPath.size() >= sizeof(addr.sun_path))
    {
        LOG_ERROR("IpcClient socket path too long: {}", m_socketPath);
        ::close(fd);
        throw std::runtime_error("IpcClient: socket path too long");
    }

    std::strncpy(addr.sun_path, m_socketPath.c_str(), sizeof(addr.sun_path) - 1);

    if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
    {
        LOG_ERROR("IpcClient connect() failed ({}): {}", m_socketPath, std::strerror(errno));
        ::close(fd);
        throw std::runtime_error("IpcClient: connect() failed");
    }

    uint16_t nlen = htons(static_cast<uint16_t>(payload.size()));
    if (!writeAll(fd, &nlen, sizeof(nlen)))
    {
        LOG_ERROR("IpcClient write(header) failed: {}", std::strerror(errno));
        ::close(fd);
        throw std::runtime_error("IpcClient: write() failed");
    }

    if (!payload.empty() && !writeAll(fd, payload.data(), payload.size()))
    {
        LOG_ERROR("IpcClient write(body) failed: {}", std::strerror(errno));
        ::close(fd);
        throw std::runtime_error("IpcClient: write() failed");
    }

    uint16_t rnlen = 0;
    if (!readExact(fd, &rnlen, sizeof(rnlen)))
    {
        LOG_ERROR("IpcClient read(header) failed: {}", std::strerror(errno));
        ::close(fd);
        throw std::runtime_error("IpcClient: read() failed");
    }

    uint16_t rlen = ntohs(rnlen);
    if (rlen > IpcFraming::MAX_BODY)
    {
        ::close(fd);
        throw std::runtime_error("IpcClient: invalid reply length");
    }

    std::string reply;
    reply.resize(rlen);

    if (rlen > 0 && !readExact(fd, reply.data(), rlen))
    {
        LOG_ERROR("IpcClient read(body) failed: {}", std::strerror(errno));
        ::close(fd);
        throw std::runtime_error("IpcClient: read() failed");
    }

    ::close(fd);
    return reply;
}
