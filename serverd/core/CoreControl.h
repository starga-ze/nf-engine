#pragma once

#include <cstdint>
#include <vector>
#include <string>

struct EngineSnapshot
{
    uint64_t uptimeSeconds;

    uint64_t rssMB;
    uint64_t heapMB;
    uint64_t dataMB;
    uint64_t virtualMB;

    double cpuPercent;
};

struct SessionInfoView 
{
    uint64_t sessionId;
    std::string state;
    int tlsFd;
    int tcpFd;
    int udpFd;
};

struct SessionSnapshot
{
    uint64_t totalSessions = 0;
    std::vector<SessionInfoView> sessions;
};

class CoreControl 
{
public:
    explicit CoreControl(class Core& core);

    SessionSnapshot sessionSnapshot();
    EngineSnapshot engineSnapshot() const;

private:
    Core& m_core;
};
