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

    return R"({"ok":false,"error":"unknown route"})";
}

std::string IpcCommandHandler::handleStatsSession()
{
    return m_client.send(R"({"cmd":"stats/session"})");
}

std::string IpcCommandHandler::handleStatsEngine()
{
    return m_client.send(R"({"cmd":"stats/engine"})");
}
