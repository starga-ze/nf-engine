#pragma once

#include <string>
#include <nlohmann/json.hpp>

class CoreControl;

class IpcCommandHandler
{
public:
    explicit IpcCommandHandler(CoreControl& control);

    std::string handle(const std::string& request);

private:
    std::string handleSessionStats();
    std::string handleEngineStats();
    std::string handleShardStats();
    std::string handleMarketStats();

    CoreControl& m_control;
};
