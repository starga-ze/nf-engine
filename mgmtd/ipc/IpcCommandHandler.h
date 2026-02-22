#pragma once

#include <string>

class IpcClient;

class IpcCommandHandler
{
public:
    explicit IpcCommandHandler(IpcClient& client);

    std::string handle(const std::string& route);

private:
    std::string handleStatsSession();
    std::string handleStatsEngine();
    std::string handleStatsShard();
    std::string handleStatsMarket();

    IpcClient& m_client;
};
