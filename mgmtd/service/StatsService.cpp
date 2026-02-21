#include "service/StatsService.h"
#include "ipc/IpcCommandHandler.h"

StatsService::StatsService(IpcCommandHandler& handler)
    : m_handler(handler)
{
}

std::string StatsService::fetchSession()
{
    return m_handler.handle("stats/session");
}

std::string StatsService::fetchEngine()
{
    return m_handler.handle("stats/engine");
}

std::string StatsService::fetchShard()
{
    return m_handler.handle("stats/shard");
}
