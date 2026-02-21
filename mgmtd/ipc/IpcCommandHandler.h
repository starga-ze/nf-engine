#pragma once

#include <string>

class IpcClient;

class IpcCommandHandler
{
public:
    explicit IpcCommandHandler(IpcClient& client);

    // REST-like routing: stats/session, stats/engine
    std::string handle(const std::string& route);

private:
    std::string handleStatsSession();
    std::string handleStatsEngine();

    IpcClient& m_client;
};
