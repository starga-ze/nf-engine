#include "ipc/IpcClient.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>

IpcClient::IpcClient(std::string path)
    : m_socketPath(std::move(path))
{
}

std::string IpcClient::send(const std::string& payload)
{
    int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0)
        throw std::runtime_error("IpcClient: socket() failed");

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, m_socketPath.c_str(), sizeof(addr.sun_path) - 1);

    if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
    {
        ::close(fd);
        throw std::runtime_error("IpcClient: connect() failed");
    }

    ssize_t w = ::write(fd, payload.data(), payload.size());
    if (w < 0)
    {
        ::close(fd);
        throw std::runtime_error("IpcClient: write() failed");
    }

    char buffer[8192];
    int n = ::read(fd, buffer, sizeof(buffer));

    ::close(fd);

    if (n <= 0)
        throw std::runtime_error("IpcClient: read() failed");

    return std::string(buffer, n);
}
