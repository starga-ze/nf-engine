#include "CoreControl.h"
#include "Core.h"

#include <chrono>
#include <fstream>
#include <unistd.h>
#include <malloc.h>

CoreControl::CoreControl(Core& core)
    : m_core(core)
{
}

EngineSnapshot CoreControl::engineSnapshot() const
{
    EngineSnapshot snapshot{};

    auto now  = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(
        now - m_core.getStartTime());

    snapshot.uptimeSeconds = diff.count();

    std::ifstream status("/proc/self/status");
    std::string line;

    while (std::getline(status, line))
    {
        if (line.rfind("VmRSS:", 0) == 0)
        {
            snapshot.rssMB = std::stoull(line.substr(6)) / 1024;
        }
        else if (line.rfind("VmData:", 0) == 0)
        {
            snapshot.dataMB = std::stoull(line.substr(7)) / 1024;
        }
        else if (line.rfind("VmSize:", 0) == 0)
        {
            snapshot.virtualMB = std::stoull(line.substr(7)) / 1024;
        }
    }

    struct mallinfo2 mi = mallinfo2();
    snapshot.heapMB = mi.uordblks / (1024 * 1024);

    static long lastTotal = 0;
    static long lastProc  = 0;

    std::ifstream stat("/proc/stat");
    std::string cpu;
    long user, nice, system, idle;

    stat >> cpu >> user >> nice >> system >> idle;
    long total = user + nice + system + idle;

    std::ifstream self("/proc/self/stat");
    std::string tmp;
    long utime, stime;

    for (int i = 0; i < 13; ++i)
        self >> tmp;

    self >> utime >> stime;

    long proc = utime + stime;

    long deltaTotal = total - lastTotal;
    long deltaProc  = proc  - lastProc;

    if (deltaTotal > 0)
        snapshot.cpuPercent = (100.0 * deltaProc) / deltaTotal;
    else
        snapshot.cpuPercent = 0.0;

    lastTotal = total;
    lastProc  = proc;

    return snapshot;
}

SessionSnapshot CoreControl::sessionSnapshot() const
{
    SessionSnapshot snapshot;

    auto* sessionManager = m_core.getSessionManager();
    if (!sessionManager)
        return snapshot;

    snapshot.totalSessions = sessionManager->totalCount();
    snapshot.sessions      = sessionManager->snapshot();

    return snapshot;
}

ShardSnapshot CoreControl::shardSnapshot() const
{
    ShardSnapshot snapshot;

    auto* shardManager = m_core.getShardManager();
    if (not shardManager)
    {
        return snapshot;
    }

    snapshot.shardCount = shardManager->getWorkerCount();

    return snapshot;
}
