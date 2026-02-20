#pragma once
#include "../ipc/UdsClient.h"

class StatsService {
public:
    explicit StatsService(std::shared_ptr<UdsClient> client)
        : m_client(std::move(client)) {}

    Stats fetch() {
        return m_client->getStats();
    }

private:
    std::shared_ptr<UdsClient> m_client;
};

