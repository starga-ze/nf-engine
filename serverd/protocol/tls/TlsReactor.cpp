#include "protocol/tls/TlsReactor.h"

#include "protocol/tls/TlsConnection.h"
#include "protocol/tls/TlsFraming.h"
#include "protocol/tls/TlsHandover.h"
#include "protocol/tls/TlsHandshake.h"

#include "packet/Packet.h"
#include "util/Logger.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/epoll.h>

#include <unordered_set>
#include <queue>
#include <vector>

#define TLS_MAX_EVENTS (64)
#define TLS_MAX_RX_BUFFER_SIZE (65536)
#define TLS_MPSC_QUEUE_SIZE (65536)

TlsReactor::TlsReactor(SSL_CTX* ctx,
                       std::vector<std::unique_ptr<TlsWorker>>& tlsWorkers,
                       std::shared_ptr<TlsHandover> handover) :
    m_ctx(ctx),
    m_handover(handover)
{
    m_tlsWorkers.reserve(tlsWorkers.size());
    for (const auto& w : tlsWorkers)
    {
        m_tlsWorkers.push_back(w.get());
    }

    m_epoll = std::make_unique<Epoll>();
    m_txQueue = std::make_unique<MpscQueue>(TLS_MPSC_QUEUE_SIZE);
}

TlsReactor::~TlsReactor()
{
    stop();
}

void TlsReactor::start()
{
    if (not m_epoll->init())
    {
        LOG_ERROR("TlsReactor epoll init failed");
        return;
    }

    m_running = true;

    std::vector<epoll_event> events(TLS_MAX_EVENTS);

    while (m_running)
    {
        int n = m_epoll->wait(events, -1);
        if (n < 0)
        {
            continue;
        }

        for (int i = 0; i < n; ++i)
        {
            int fd = events[i].data.fd;
            uint32_t ev = events[i].events;

            if (fd == m_epoll->getWakeupFd())
            {
                m_epoll->drainWakeup();
                processWakeup();
                continue;
            }

            if (ev & (EPOLLERR | EPOLLHUP | EPOLLRDHUP))
            {
                closeConnection(fd);
                continue;
            }

            if (ev & EPOLLIN)
            {
                onReadable(fd);
            }

            if (ev & EPOLLOUT)
            {
                onWritable(fd);
            }
        }
    }

    shutdown();
}

void TlsReactor::stop()
{
    m_running = false;
    if (m_epoll)
    {
        m_epoll->wakeup();
    }
}

void TlsReactor::shutdown()
{
    for (auto& kv : m_conns)
    {
        ::close(kv.first);
    }
    m_conns.clear();

    if (m_epoll)
    {
        m_epoll->close();
    }
}

bool TlsReactor::setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    return flags >= 0 && fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0;
}

void TlsReactor::handover(int fd, std::pair<sockaddr_in, sockaddr_in> connInfo)
{
    if (!setNonBlocking(fd))
    {
        ::close(fd);
        return;
    }

    m_handover->push(fd, connInfo);
    m_epoll->wakeup();
}

void TlsReactor::enqueueTx(std::unique_ptr<Packet> pkt)
{
    if (!pkt)
    {
        return;
    }

    if (m_txQueue->enqueue(std::move(pkt)))
    {
        m_epoll->wakeup();
    }
    else
    {
        LOG_WARN("TlsReactor Tx MPSC Queue Full, pkt dropped");
    }
}

void TlsReactor::processWakeup()
{
    processHandover();
    processTxQueue();
}

void TlsReactor::processHandover()
{
    std::queue<TlsHandoverItem> q;
    m_handover->popAll(q);

    while (!q.empty())
    {
        auto item = q.front();
        q.pop();

        int fd = item.fd;

        SSL* ssl = SSL_new(m_ctx);
        if (!ssl)
        {
            ::close(fd);
            continue;
        }

        SSL_set_accept_state(ssl);
        SSL_set_fd(ssl, fd);

        auto conn = std::make_unique<TlsConnection>(fd, ssl, item.connInfo, TLS_MAX_RX_BUFFER_SIZE);
        m_conns.emplace(fd, std::move(conn));

        m_epoll->add(fd, EPOLLIN | EPOLLRDHUP);
    }
}

void TlsReactor::processTxQueue()
{
    std::vector<std::unique_ptr<Packet>> packets;
    m_txQueue->dequeueAll(packets);

    if (packets.empty())
    {
        return;
    }

    std::unordered_set<int> dirtyFds;

    for (auto& pkt : packets)
    {
        int fd = pkt->getFd();
        auto it = m_conns.find(fd);
        if (it == m_conns.end())
        {
            continue;
        }

        it->second->txQ().push_back(std::move(pkt));
        dirtyFds.insert(fd);
    }

    for (int fd : dirtyFds)
    {
        updateInterest(fd);
        flushTx(fd, 256);
    }
}

bool TlsReactor::isHandshakeDone(int fd) const
{
    auto it = m_conns.find(fd);
    if (it == m_conns.end())
    {
        return false;
    }
    return SSL_is_init_finished(it->second->ssl()) == 1;
}

bool TlsReactor::driveHandshake(int fd)
{
    auto it = m_conns.find(fd);
    if (it == m_conns.end())
    {
        return false;
    }

    SSL* ssl = it->second->ssl();
    TlsHandshakeStep s = TlsHandshake::drive(ssl);

    if (s == TlsHandshakeStep::Done)
    {
        updateInterest(fd);
        receive(fd);
        return true;
    }
    if (s == TlsHandshakeStep::WantRead)
    {
        m_epoll->mod(fd, EPOLLIN | EPOLLRDHUP);
        return true;
    }
    if (s == TlsHandshakeStep::WantWrite)
    {
        m_epoll->mod(fd, EPOLLIN | EPOLLOUT | EPOLLRDHUP);
        return true;
    }

    closeConnection(fd);
    return false;
}

void TlsReactor::onReadable(int fd)
{
    auto it = m_conns.find(fd);
    if (it == m_conns.end())
    {
        return;
    }

    if (!isHandshakeDone(fd))
    {
        driveHandshake(fd);
        return;
    }

    receive(fd);
}

void TlsReactor::onWritable(int fd)
{
    auto it = m_conns.find(fd);
    if (it == m_conns.end())
    {
        return;
    }

    if (!isHandshakeDone(fd))
    {
        driveHandshake(fd);
        return;
    }

    flushTx(fd, 256);
    updateInterest(fd);
}

void TlsReactor::receive(int fd)
{
    auto it = m_conns.find(fd);
    if (it == m_conns.end())
    {
        return;
    }

    auto& conn = it->second;
    SSL* ssl = conn->ssl();
    ByteRingBuffer& rxRing = conn->rxRing();

    while (true)
    {
        uint8_t* wptr = rxRing.writePtr();
        size_t wlen = rxRing.writeLen();

        if (wlen == 0)
        {
            closeConnection(fd);
            return;
        }

        int n = SSL_read(ssl, wptr, (int)wlen);

        if (n > 0)
        {
            rxRing.produce((size_t)n);

            while (true)
            {
                size_t frameLen = 0;
                TlsFramingResult r = TlsFraming::tryExtractFrame(rxRing, frameLen);

                if (r == TlsFramingResult::NeedMoreData)
                {
                    break;
                }
                if (r != TlsFramingResult::Ok)
                {
                    closeConnection(fd);
                    return;
                }

                std::vector<uint8_t> payload(frameLen);
                size_t rb = rxRing.read(payload.data(), frameLen);
                if (rb != frameLen)
                {
                    closeConnection(fd);
                    return;
                }

                auto pkt = std::make_unique<Packet>(fd, Protocol::TLS, std::move(payload),
                                                    conn->peerAddr(), conn->serverAddr());

                size_t idx = (size_t)pkt->getFd() % m_tlsWorkers.size();
                m_tlsWorkers[idx]->enqueueRx(std::move(pkt));
            }

            continue;
        }

        int err = SSL_get_error(ssl, n);

        if (err == SSL_ERROR_WANT_READ)
        {
            return;
        }
        if (err == SSL_ERROR_WANT_WRITE)
        {
            m_epoll->mod(fd, EPOLLIN | EPOLLOUT | EPOLLRDHUP);
            return;
        }

        closeConnection(fd);
        return;
    }
}

void TlsReactor::flushTx(int fd, size_t budget)
{
    auto it = m_conns.find(fd);
    if (it == m_conns.end())
    {
        return;
    }

    auto& conn = it->second;
    SSL* ssl = conn->ssl();
    auto& q = conn->txQ();

    size_t used = 0;

    while (used < budget)
    {
        if (q.empty())
        {
            return;
        }

        auto pkt = std::move(q.front());
        q.pop_front();

        const auto& payload = pkt->getPayload();

        while (pkt->getTxOffset() < payload.size())
        {
            const uint8_t* p = payload.data() + pkt->getTxOffset();
            size_t bytes = payload.size() - pkt->getTxOffset();

            int ret = SSL_write(ssl, p, (int)bytes);

            if (ret > 0)
            {
                pkt->updateTxOffset((size_t)ret);
                continue;
            }

            int err = SSL_get_error(ssl, ret);

            if (err == SSL_ERROR_WANT_WRITE || err == SSL_ERROR_WANT_READ)
            {
                q.push_front(std::move(pkt));
                m_epoll->mod(fd, EPOLLIN | EPOLLOUT | EPOLLRDHUP);
                return;
            }

            closeConnection(fd);
            return;
        }

        used++;
    }
}

void TlsReactor::updateInterest(int fd)
{
    auto it = m_conns.find(fd);
    if (it == m_conns.end())
    {
        return;
    }

    uint32_t ev = EPOLLIN | EPOLLRDHUP;

    if (!isHandshakeDone(fd))
    {
        ev |= EPOLLOUT;
        m_epoll->mod(fd, ev);
        return;
    }

    if (!it->second->txQ().empty())
    {
        ev |= EPOLLOUT;
    }

    m_epoll->mod(fd, ev);
}

void TlsReactor::closeConnection(int fd)
{
    m_epoll->del(fd);
    ::close(fd);
    m_conns.erase(fd);
}
