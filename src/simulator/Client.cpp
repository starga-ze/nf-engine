#include "Client.h"
#include "util/Logger.h"

#include <thread>
#include <chrono>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <cstdint>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

Client::Client(int id, int udpServerPort, int tcpServerPort)
    : m_id(id),
      m_udpServerPort(udpServerPort),
      m_tcpServerPort(tcpServerPort)
{
    try
    {
        m_logger = spdlog::basic_logger_mt(
            "nf-client-" + std::to_string(id),
            "/var/log/nf/nf-client.log"
        );

        m_logger->set_pattern(
            "[%Y-%m-%d %H:%M:%S.%e][%l][client=%t] %v"
        );
        m_logger->set_level(spdlog::level::info);
        m_logger->flush_on(spdlog::level::info);
    }
    catch (const spdlog::spdlog_ex& e)
    {
        // fallback: global logger
        spdlog::error("Client logger init failed: {}", e.what());
        m_logger = spdlog::default_logger();
    }

    init();
}

Client::~Client()
{
    stop();
}

void Client::init()
{
    m_udpClient = std::make_unique<UdpClient>(m_id, m_udpServerPort);
    m_tcpClient = std::make_unique<TcpClient>(m_id, m_tcpServerPort);
    m_tlsClient = std::make_unique<TlsClient>(m_id, m_tcpServerPort);
}

void Client::start()
{
    if (!m_tlsClient->connect())
    {
        LOG_FATAL("TLS connect failed");
        return;
    }

    if (!m_tcpClient->connect())
    {
        LOG_FATAL("TCP connect failed");
        return;
    }

    if (!m_udpClient->open())
    {
        LOG_FATAL("UDP open failed");
        return;
    }

    loginPhase();

    m_running = true;

    while (m_running)
    {
        pingPhase();
        lobbyPhase();
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    }
}

void Client::loginPhase()
{
    auto pkt = buildLoginReq();

    if (!m_tlsClient->send(pkt.data(), pkt.size())) {
        LOG_FATAL("LOGIN_REQ send failed");
        return;
    }

    uint8_t buf[1024];
    ssize_t n;

    while (true) {
        n = m_tlsClient->recv(buf, sizeof(buf));
        if (n > 0)
            break;
        if (n < 0) {
            LOG_FATAL("TLS recv fail");
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    uint64_t sessionId = 0;
    std::memcpy(&sessionId, buf + 4, sizeof(sessionId));
    m_sessionId = be64toh(sessionId);
    return;
}

void Client::lobbyPhase()
{
    auto pkt = buildLobbyEntryReq();

    if (not m_tcpClient->send(pkt.data(), pkt.size())) {
        LOG_FATAL("LOBBY_ENTRY_REQ send failed");
        return;
    }

    uint8_t buf[1024];
    ssize_t n;

    while(true) {
        n = m_tcpClient->recv(buf, sizeof(buf));
        if (n > 0)
        {
            break;
        }
        if (n < 0)
        {
            LOG_FATAL("TCP recv fail");
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void Client::pingPhase()
{
    constexpr auto PING_INTERVAL = std::chrono::milliseconds(5000);

    auto now = std::chrono::steady_clock::now();

    if (now - m_lastPingTs < PING_INTERVAL)
    {
        return;
    }

    m_lastPingTs = now;

    auto pkt = buildPingReq();

    if (!m_udpClient->send(pkt.data(), pkt.size()))
    {
        LOG_ERROR("UDP ping send failed");
        return;
    }
    m_logger->info("client send ping");

    uint8_t buf[1024];

    if (!m_udpClient->recv(buf, sizeof(buf), 1000))
    {
        LOG_ERROR("UDP ping recv failed");
    }
    m_logger->info("client recv ping");
}


void Client::stop()
{
    m_running.store(false);
    m_tlsClient->stop();
    m_tcpClient->stop();
    m_udpClient->stop();
}

std::vector<uint8_t> Client::buildLoginReq()
{
    const char* id = "test";
    const char* pw = "test";

    uint16_t bodyLen =
        2 + std::strlen(id) +
        2 + std::strlen(pw);

    std::vector<uint8_t> pkt(16 + bodyLen);

    pkt[0] = 0x01;  
    pkt[1] = 0x10;  
    pkt[2] = bodyLen >> 8;
    pkt[3] = bodyLen & 0xFF;

    uint64_t sid = 0;
    std::memcpy(pkt.data() + 4, &sid, sizeof(sid));

    std::memset(pkt.data() + 12, 0, 4);

    size_t off = 16;

    uint16_t idLen = std::strlen(id);
    pkt[off++] = idLen >> 8;
    pkt[off++] = idLen & 0xFF;
    std::memcpy(pkt.data() + off, id, idLen);
    off += idLen;

    uint16_t pwLen = std::strlen(pw);
    pkt[off++] = pwLen >> 8;
    pkt[off++] = pwLen & 0xFF;
    std::memcpy(pkt.data() + off, pw, pwLen);

    return pkt;
}

std::vector<uint8_t> Client::buildLobbyEntryReq()
{
    std::vector<uint8_t> pkt(16);

    pkt[0] = 0x01;                    
    pkt[1] = 0x20;  
    pkt[2] = 0x00;                    
    pkt[3] = 0x00;  

    uint64_t sessionId = htobe64(m_sessionId);
    std::memcpy(pkt.data() + 4, &sessionId, sizeof(sessionId));

    std::memset(pkt.data() + 12, 0, 4);

    return pkt;
}

std::vector<uint8_t> Client::buildPingReq()
{
    constexpr uint16_t BODY_LEN = 16;

    std::vector<uint8_t> pkt(16 + BODY_LEN);

    pkt[0] = 0x01;
    pkt[1] = 0x90;
    pkt[2] = (BODY_LEN >> 8) & 0xFF;
    pkt[3] = BODY_LEN & 0xFF;

    uint64_t sid = htobe64(m_sessionId);
    std::memcpy(pkt.data() + 4, &sid, 8);
    std::memset(pkt.data() + 12, 0, 4);

    size_t off = 16;

    uint64_t nonce = ++m_pingSeq;
    uint64_t nonce_be = htobe64(nonce);
    std::memcpy(pkt.data() + off, &nonce_be, 8);
    off += 8;

    uint64_t ts =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();

    uint64_t ts_be = htobe64(ts);
    std::memcpy(pkt.data() + off, &ts_be, 8);

    return pkt;
}


void Client::dumpHex(const uint8_t* buf, size_t len)
{
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');

    for (size_t i = 0; i < len; i++) {
        oss << std::setw(2) << (int)buf[i] << ' ';
        if ((i + 1) % 16 == 0)
            oss << '\n';
    }

    LOG_TRACE("Client rx dump (len={}):\n{}", len, oss.str());
}

