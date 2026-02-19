#include "UdpEpoll.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <errno.h>

UdpEpoll::UdpEpoll() = default;

UdpEpoll::~UdpEpoll()
{
    close();
}

bool UdpEpoll::init()
{
    if (not createEpoll()) return false;
    if (not createEventFd()) return false;

    add(m_eventFd, EPOLLIN);
    return true;
}

void UdpEpoll::close()
{
    if (m_epFd >= 0)
    {
        ::close(m_epFd);
        m_epFd = -1;
    }
    if (m_eventFd >= 0)
    {
        ::close(m_eventFd);
        m_eventFd = -1;
    }
}

bool UdpEpoll::createEpoll()
{
    m_epFd = ::epoll_create1(0);
    return m_epFd >= 0;
}

bool UdpEpoll::createEventFd()
{
    m_eventFd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    return m_eventFd >= 0;
}

bool UdpEpoll::add(int fd, uint32_t events)
{
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    return ::epoll_ctl(m_epFd, EPOLL_CTL_ADD, fd, &ev) == 0;
}

bool UdpEpoll::mod(int fd, uint32_t events)
{
    epoll_event ev{};
    ev.events = events;
    ev.data.fd = fd;
    return ::epoll_ctl(m_epFd, EPOLL_CTL_MOD, fd, &ev) == 0;
}

bool UdpEpoll::del(int fd)
{
    return ::epoll_ctl(m_epFd, EPOLL_CTL_DEL, fd, nullptr) == 0;
}

int UdpEpoll::wait(std::vector<epoll_event>& events, int timeoutMs)
{
    return ::epoll_wait(m_epFd, events.data(), (int)events.size(), timeoutMs);
}

void UdpEpoll::wakeup()
{
    if (m_eventFd < 0) return;
    uint64_t v = 1;
    (void)::write(m_eventFd, &v, sizeof(v));
}

void UdpEpoll::drainWakeup()
{
    uint64_t v;
    while (true)
    {
        ssize_t n = ::read(m_eventFd, &v, sizeof(v));
        if (n == (ssize_t)sizeof(v)) continue;
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) break;
        break;
    }
}

