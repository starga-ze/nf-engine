#pragma once

#include <queue>
#include <mutex>
#include <utility>
#include <netinet/in.h>

struct TlsHandoverItem
{
    int fd;
    std::pair<sockaddr_in, sockaddr_in> connInfo;
};

class TlsHandover
{
public:
    void push(int fd, std::pair<sockaddr_in, sockaddr_in> connInfo);
    void popAll(std::queue<TlsHandoverItem>& out);

private:
    std::mutex m_lock;
    std::queue<TlsHandoverItem> m_q;
};
