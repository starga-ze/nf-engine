#include "ipc/IpcServer.h"
#include "io/Epoll.h"

#include <nlohmann/json.hpp>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <vector>

using json = nlohmann::json;

static void setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

IpcServer::IpcServer(CoreControl& control, std::string socketPath) : 
    m_control(control),
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
    if (m_serverFd < 0) {
        std::cerr << "[IPC] socket() failed\n";
        return false;
    }

    setNonBlocking(m_serverFd);

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, m_socketPath.c_str(),
                 sizeof(addr.sun_path) - 1);

    if (::bind(m_serverFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
    {
        std::cerr << "[IPC] bind() failed\n";
        ::close(m_serverFd);
        m_serverFd = -1;
        return false;
    }

    ::chmod(m_socketPath.c_str(), 0666);

    if (::listen(m_serverFd, 16) < 0) {
        std::cerr << "[IPC] listen() failed\n";
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

    if (not m_ipcEpoll->init())
        return;

    if (!setupSocket())
        return;

    m_ipcEpoll->add(m_serverFd, EPOLLIN);

    std::vector<epoll_event> events(16);

    while (m_running) {

        int n = m_ipcEpoll->wait(events, -1);
        if (n <= 0)
            continue;

        for (int i = 0; i < n; ++i) {

            int fd = events[i].data.fd;

            if (fd == m_ipcEpoll->getWakeupFd()) {
                m_ipcEpoll->drainWakeup();
                m_running = false;
                break;
            }

            if (fd == m_serverFd) {
                handleAccept();
            }
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
            break;

        setNonBlocking(clientFd);

        char buffer[2048] = {0};
        ssize_t len = ::read(clientFd, buffer, sizeof(buffer) - 1);

        std::string reply;

        if (len > 0)
            reply = m_handler->handle(std::string(buffer, len));
        else
            reply = R"({"ok":false,"error":"empty request"})";

        ::send(clientFd, reply.c_str(), reply.size(), MSG_NOSIGNAL);
        ::close(clientFd);
    }
}

void IpcServer::stop()
{
    if (!m_running)
        return;

    m_running = false;

    if (m_ipcEpoll) {
        m_ipcEpoll->wakeup();
    }
}

void IpcServer::cleanup()
{
    if (m_serverFd >= 0) {
        ::close(m_serverFd);
        m_serverFd = -1;
    }

    ::unlink(m_socketPath.c_str());
}

