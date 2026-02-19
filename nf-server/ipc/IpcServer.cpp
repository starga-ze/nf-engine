#include "ipc/IpcServer.h"
#include "io/Epoll.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <vector>

static void setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

IpcServer::IpcServer(std::string socketPath)
    : m_socketPath(std::move(socketPath))
{
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

    if (::bind(m_serverFd,
               reinterpret_cast<sockaddr*>(&addr),
               sizeof(addr)) < 0)
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

            // wakeup eventfd
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
    while (true) {

        int clientFd = ::accept(m_serverFd, nullptr, nullptr);
        if (clientFd < 0)
            break;

        setNonBlocking(clientFd);

        char buffer[1024];
        ::read(clientFd, buffer, sizeof(buffer));

        const char* response =
            "{\"rx_packets\":1000,"
            "\"tx_packets\":2000,"
            "\"active_sessions\":5}";

        ::send(clientFd,
               response,
               std::strlen(response),
               MSG_NOSIGNAL);

        ::close(clientFd);
    }
}

void IpcServer::stop()
{
    if (!m_running)
        return;

    m_running = false;

    if (m_ipcEpoll) {
        m_ipcEpoll->wakeup();   // eventfd write
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

