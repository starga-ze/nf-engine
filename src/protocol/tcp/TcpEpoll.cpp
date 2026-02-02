#include "TcpEpoll.h"

#include "util/Logger.h"

#include <unistd.h>
#include <sys/eventfd.h>
#include <errno.h>

TcpEpoll::TcpEpoll() = default;

TcpEpoll::~TcpEpoll()
{
    close();
}

bool TcpEpoll::init()
{
    if (!createEpoll())
        return false;

    if (!createEventFd())
        return false;

    add(m_eventFd, EPOLLIN);
    return true;
}

bool TcpEpoll::createEpoll()
{
    m_epFd = ::epoll_create1(0);
    if (m_epFd < 0) {
        LOG_ERROR("epoll_create1 failed errno={}", errno);
        return false;
    }
    return true;
}

bool TcpEpoll::createEventFd()
{
    m_eventFd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (m_eventFd < 0) {
        LOG_ERROR("eventfd failed errno={}", errno);
        return false;
    }
    return true;
}

void TcpEpoll::close()
{
    if (m_eventFd >= 0) {
        ::close(m_eventFd);
        m_eventFd = -1;
    }

    if (m_epFd >= 0) {
        ::close(m_epFd);
        m_epFd = -1;
    }
}

bool TcpEpoll::add(int fd, uint32_t events)
{
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    return ::epoll_ctl(m_epFd, EPOLL_CTL_ADD, fd, &ev) == 0;
}

bool TcpEpoll::mod(int fd, uint32_t events)
{
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    return ::epoll_ctl(m_epFd, EPOLL_CTL_MOD, fd, &ev) == 0;
}

bool TcpEpoll::del(int fd)
{
    return ::epoll_ctl(m_epFd, EPOLL_CTL_DEL, fd, nullptr) == 0;
}

int TcpEpoll::wait(std::vector<epoll_event>& events, int timeoutMs)
{
    return ::epoll_wait(m_epFd, events.data(), 
            static_cast<int>(events.size()), timeoutMs);
}

void TcpEpoll::wakeup()
{
    uint64_t v = 1;
    ::write(m_eventFd, &v, sizeof(v));
}

void TcpEpoll::drainWakeup()
{
    uint64_t v;
    while (::read(m_eventFd, &v, sizeof(v)) > 0) {}
}

