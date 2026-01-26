#include "Client.h"
#include "util/Logger.h"

#include <thread>
#include <chrono>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <cstdint>

Client::Client(int id, int udpServerPort, int tcpServerPort)
    : m_id(id),
      m_udpServerPort(udpServerPort),
      m_tcpServerPort(tcpServerPort)
{
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
    if (not m_tlsClient->connect())
    {
        LOG_FATAL("TLS connect failed");
        return;
    }

    loginPhase();

    LOG_INFO("SessionId; {}", m_sessionId);

    m_running = true;
    while(m_running)
    {
        std::this_thread::sleep_for(std::chrono::seconds(3));
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
            LOG_FATAL("TLS recv fatal");
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    //dumpHex(buf, n);

    uint64_t sessionId = 0;
    std::memcpy(&sessionId, buf + 4, sizeof(sessionId));
    m_sessionId = be64toh(sessionId);
    return;
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
    static const uint8_t LOGIN_REQ_TEST_PACKET[28] =
    {
        0x01, 0x10, 0x00, 0x0C, // ver + opcode + bodylen
        0x00, 0x00, 0x00, 0x00, // sessionId
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, // flags
        0x00, 0x04, 0x74, 0x65, // body
        0x73, 0x74, 0x00, 0x04,
        0x74, 0x65, 0x73, 0x74
    };

    return std::vector<uint8_t>(
        LOGIN_REQ_TEST_PACKET,
        LOGIN_REQ_TEST_PACKET + sizeof(LOGIN_REQ_TEST_PACKET)
    );
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

