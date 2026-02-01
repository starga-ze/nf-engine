#pragma once

#include "simulator/UdpClient.h"
#include "simulator/TcpClient.h"
#include "simulator/TlsClient.h"

#include <atomic>
#include <memory>
#include <vector>

enum class ProtocolType {
    TCP,
    UDP,
    TLS
};

struct SimPacket {
    ProtocolType protocol;
    std::vector<uint8_t> data;
};

class Client
{
public:
    Client(int id, int udpServerPort, int tcpServerPort);
    ~Client();

    void start();
    void stop();

private:
    void dumpHex(const uint8_t* buf, size_t len);
    void init();

    void loginPhase();
    void lobbyPhase();

    std::vector<uint8_t> buildLoginReq();
    std::vector<uint8_t> buildLobbyEntryReq();

    std::unique_ptr<TcpClient> m_tcpClient;
    std::unique_ptr<UdpClient> m_udpClient;
    std::unique_ptr<TlsClient> m_tlsClient;

    std::atomic<bool> m_running {false};

    int m_id;
    int m_udpServerPort;
    int m_tcpServerPort;

    uint64_t m_sessionId;
};

