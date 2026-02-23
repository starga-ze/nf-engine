#include "ipc/IpcServer.h"
#include "io/Epoll.h"
#include "ipc/IpcFraming.h"
#include "util/Logger.h"

#include <nlohmann/json.hpp>

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <string>
#include <vector>

using json = nlohmann::json;

static void setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0)
        (void)fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

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

static bool sendFramed(int fd, const std::string& payload)
{
    if (payload.size() > IpcFraming::MAX_BODY)
        return false;

    uint16_t nlen = htons(static_cast<uint16_t>(payload.size()));
    if (!writeAll(fd, &nlen, sizeof(nlen)))
        return false;

    if (!payload.empty() && !writeAll(fd, payload.data(), payload.size()))
        return false;

    return true;
}

static bool recvFramed(int fd, std::string& outPayload)
{
    uint16_t nlen = 0;
    if (!readExact(fd, &nlen, sizeof(nlen)))
        return false;

    uint16_t len = ntohs(nlen);
    if (len > IpcFraming::MAX_BODY)
        return false;

    outPayload.clear();
    outPayload.resize(len);

    if (len > 0 && !readExact(fd, outPayload.data(), len))
        return false;

    return true;
}

IpcServer::IpcServer(CoreControl& control, std::string socketPath)
    : m_control(control),
      m_socketPath(std::move(socketPath))
{
    m_handler = std::make_unique<IpcCommandHandler>(control);
}

IpcServer::~IpcServer()
{
    cleanup();
}

bool IpcServer::setupSocket()
{
    ::unlink(m_socketPath.c_str());

    m_serverFd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_serverFd < 0)
    {
        LOG_ERROR("[IPC] socket() failed: {}", std::strerror(errno));
        return false;
    }

    setNonBlocking(m_serverFd);

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;

    if (m_socketPath.size() >= sizeof(addr.sun_path))
    {
        LOG_ERROR("[IPC] socket path too long: {}", m_socketPath);
        ::close(m_serverFd);
        m_serverFd = -1;
        return false;
    }

    std::strncpy(addr.sun_path, m_socketPath.c_str(), sizeof(addr.sun_path) - 1);

    if (::bind(m_serverFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
    {
        LOG_ERROR("[IPC] bind() failed ({}): {}", m_socketPath, std::strerror(errno));
        ::close(m_serverFd);
        m_serverFd = -1;
        return false;
    }

    if (::chmod(m_socketPath.c_str(), 0666) < 0)
        LOG_WARN("[IPC] chmod(0666) failed ({}): {}", m_socketPath, std::strerror(errno));

    if (::listen(m_serverFd, 64) < 0)
    {
        LOG_ERROR("[IPC] listen() failed: {}", std::strerror(errno));
        ::close(m_serverFd);
        m_serverFd = -1;
        return false;
    }

    return true;
}

void IpcServer::start()
{
    m_running = true;

    m_ipcEpoll = std::make_unique<Epoll>();
    if (!m_ipcEpoll->init())
        return;

    if (!setupSocket())
        return;

    m_ipcEpoll->add(m_serverFd, EPOLLIN);

    std::vector<epoll_event> events(32);

    while (m_running)
    {
        int n = m_ipcEpoll->wait(events, -1);
        if (n <= 0)
            continue;

        for (int i = 0; i < n; ++i)
        {
            int fd = events[i].data.fd;

            if (fd == m_ipcEpoll->getWakeupFd())
            {
                m_ipcEpoll->drainWakeup();
                m_running = false;
                break;
            }

            if (fd == m_serverFd)
                handleAccept();
        }
    }

    cleanup();
}

void IpcServer::handleAccept()
{
    while (true)
    {
        int clientFd = ::accept(m_serverFd, nullptr, nullptr);
        if (clientFd < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                return;
            LOG_WARN("[IPC] accept() failed: {}", std::strerror(errno));
            return;
        }

        std::string req;
        std::string reply;

        if (!recvFramed(clientFd, req))
        {
            reply = R"({"ok":false,"error":"bad request"})";
            (void)sendFramed(clientFd, reply);
            ::close(clientFd);
            continue;
        }

        try
        {
            reply = m_handler->handle(req);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("[IPC] handler exception: {}", e.what());
            reply = R"({"ok":false,"error":"handler exception"})";
        }
        catch (...)
        {
            LOG_ERROR("[IPC] handler unknown exception");
            reply = R"({"ok":false,"error":"handler unknown exception"})";
        }

        if (!sendFramed(clientFd, reply))
            LOG_WARN("[IPC] send reply failed: {}", std::strerror(errno));

        ::close(clientFd);
    }
}

void IpcServer::stop()
{
    if (!m_running)
        return;

    m_running = false;

    if (m_ipcEpoll)
        m_ipcEpoll->wakeup();
}

void IpcServer::cleanup()
{
    if (m_serverFd >= 0)
    {
        ::close(m_serverFd);
        m_serverFd = -1;
    }

    ::unlink(m_socketPath.c_str());
}
