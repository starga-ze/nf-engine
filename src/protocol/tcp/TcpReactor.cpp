#include "TcpReactor.h"

#include "protocol/tcp/TcpWorker.h"
#include "protocol/tls/TlsServer.h"

#include "util/Logger.h"

#include <unistd.h> 
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>


TcpReactor::TcpReactor(int port, TcpWorker* tcpWorker, std::shared_ptr<TlsServer> tlsServer) :
    m_port(port),
    m_tcpWorker(tcpWorker),
    m_tlsServer(tlsServer)
{
    m_tcpEpoll = std::make_unique<TcpEpoll>();
}

TcpReactor::~TcpReactor()
{

}

void TcpReactor::start()
{
    if (not init())
    {
        return;
    }
    
    if (not m_tcpEpoll->init())
    {
        return;
    }

    m_tcpEpoll->add(m_listenFd, EPOLLIN);

    m_running = true;

    std::vector<epoll_event> events(64);

    while (m_running) 
    {
        int n = m_tcpEpoll->wait(events, -1);
        if (n < 0) 
        {
            continue;
        }

        for (int i = 0; i < n; ++i) 
        {
            int fd = events[i].data.fd;

            if (fd == m_tcpEpoll->getWakeupFd()) 
            {
                m_tcpEpoll->drainWakeup();
                continue;
            }

            if (fd == m_listenFd) 
            {
                LOG_DEBUG("listen fd readable (accept next)");
            }
        }
    }

    m_tcpEpoll->close();
    close();
    return;
}

void TcpReactor::stop()
{
    m_running = false;
}

bool TcpReactor::init()
{
    if (not create())
    {
        return false;
    }

    if (not setSockOpt())
    {
        return false;
    }

    if (not bindAndListen())
    {
        return false;
    }

    LOG_INFO("TCP listen ready port={}", m_port);
    return true;    
}

bool TcpReactor::create()
{
    m_listenFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (m_listenFd < 0)
    {
        LOG_ERROR("socket() failed errno={}", errno);
        return false;
    }

    if (not setNonBlocking(m_listenFd))
    {
        LOG_ERROR("setNonBlocking failed, fd={}", m_listenFd);
        return false;
    }

    return true;
}

bool TcpReactor::setSockOpt()
{
    int opt = 1;
    if (::setsockopt(m_listenFd, SOL_SOCKET,
                     SO_REUSEADDR, &opt, sizeof(opt)) != 0) {
        LOG_ERROR("setsockopt(SO_REUSEADDR) failed fd={}", m_listenFd);
        return false;
    }

    return true;
}

bool TcpReactor::bindAndListen()
{
    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_addr.s_addr = INADDR_ANY;
    m_serverAddr.sin_port = htons(m_port);

    if (::bind(m_listenFd, reinterpret_cast<sockaddr*>(&m_serverAddr), 
                sizeof(m_serverAddr)) != 0) 
    {
        LOG_ERROR("bind() failed port={} errno={}", m_port, errno);
        return false;
    }

    if (::listen(m_listenFd, SOMAXCONN) != 0) 
    {
        LOG_ERROR("listen() failed fd={} errno={}", m_listenFd, errno);
        return false;
    }

    return true;
}

void TcpReactor::close()
{
    if (m_listenFd >= 0)
    {
        ::close(m_listenFd);
        m_listenFd = -1;
    }
}

bool TcpReactor::setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
    {
        return false;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

