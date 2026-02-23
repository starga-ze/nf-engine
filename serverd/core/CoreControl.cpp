#include "CoreControl.h"
#include "Core.h"

#include "execution/market/MarketEvent.h"

#include <chrono>
#include <fstream>
#include <unistd.h>
#include <malloc.h>
#include <future>

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

MarketSnapshot CoreControl::marketSnapshot() const
{
    MarketSnapshot snapshot;

    auto* shardManager = m_core.getShardManager();

    if (not shardManager)
    {
        return snapshot;
    }

    size_t workerCount = shardManager->getWorkerCount();

    std::vector<std::future<MarketInfo>> futures;
    futures.reserve(workerCount);

    for (size_t i = 0; i < workerCount; ++i)
    {
        std::promise<MarketInfo> promise;
        futures.push_back(promise.get_future());

        auto ev = std::make_unique<MarketDumpReqEvent>(std::move(promise));
        shardManager->dispatch(i, std::move(ev));
    }

    for (auto& f : futures)
    {
        try
        {
            if (f.wait_for(std::chrono::milliseconds(500)) == std::future_status::ready)
            {
                // Blocking func, wait for promise.set_value()
                snapshot.markets.emplace_back(f.get()); 
            }
            else
            {
                LOG_WARN("MarketSnapshot timeout from shard");
            }
        }
        catch (const std::future_error& e)
        {
            LOG_ERROR("future_error in MarketSnapshot: {}", e.what());
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Exception in MarketSnapshot: {}", e.what());
        }
        catch (...)
        {
            LOG_ERROR("Unknown exception in marketsnapshot");
        }
    }
    return snapshot;
}
