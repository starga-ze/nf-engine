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

SessionSnapshot CoreControl::sessionSnapshot()
{
    SessionSnapshot snapshot;

    auto* sm = m_core.getSessionManager();
    if (!sm)
        return snapshot;

    snapshot.totalSessions = sm->totalCount();
    snapshot.sessions      = sm->snapshot();

    return snapshot;
}

EngineSnapshot CoreControl::engineSnapshot() const
{
    EngineSnapshot snapshot{};

    // ---- Uptime ----
    auto now  = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::seconds>(
        now - m_core.getStartTime());

    snapshot.uptimeSeconds = diff.count();

    // ---- /proc/self/status 기반 메모리 ----
    std::ifstream status("/proc/self/status");
    std::string line;

    while (std::getline(status, line))
    {
        if (line.rfind("VmRSS:", 0) == 0)
        {
            // VmRSS:    24832 kB
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

    // ---- Heap (glibc malloc 영역) ----
    struct mallinfo2 mi = mallinfo2();
    snapshot.heapMB = mi.uordblks / (1024 * 1024);

    // ---- CPU 사용률 ----
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
