#include "protocol/tls/TlsHandover.h"

void TlsHandover::push(int fd, std::pair<sockaddr_in, sockaddr_in> connInfo)
{
    std::lock_guard<std::mutex> g(m_lock);
    m_q.push(TlsHandoverItem{fd, connInfo});
}

void TlsHandover::popAll(std::queue<TlsHandoverItem>& out)
{
    std::lock_guard<std::mutex> g(m_lock);
    out.swap(m_q);
}
