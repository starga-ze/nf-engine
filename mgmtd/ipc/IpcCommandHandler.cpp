#include "ipc/IpcCommandHandler.h"
#include "ipc/IpcClient.h"

IpcCommandHandler::IpcCommandHandler(IpcClient& client)
    : m_client(client)
{
}

std::string IpcCommandHandler::handle(const std::string& route)
{
    if (route == "stats/session")
        return handleStatsSession();

    if (route == "stats/engine")
        return handleStatsEngine();

    if (route == "stats/shard")
        return handleStatsShard();

    if (route == "stats/market")
        return handleStatsMarket();

    return R"({"error":"unknown route"})";
}

std::string IpcCommandHandler::handleStatsSession()
{
    return m_client.send(R"({"cmd":"stats/session"})");
}

std::string IpcCommandHandler::handleStatsEngine()
{
    return m_client.send(R"({"cmd":"stats/engine"})");
}

std::string IpcCommandHandler::handleStatsShard()
{
    return m_client.send(R"({"cmd":"stats/shard"})");
}

std::string IpcCommandHandler::handleStatsMarket()
{
    return m_client.send(R"({"cmd":"stats/market"})");
}
