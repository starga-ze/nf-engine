#pragma once

#include <string>

class IpcClient
{
public:
    explicit IpcClient(std::string path);

    std::string send(const std::string& payload);

private:
    std::string m_socketPath;
};
