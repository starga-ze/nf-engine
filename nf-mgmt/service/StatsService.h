#pragma once
#include "../ipc/UdsClient.h"

class StatsService {
public:
    explicit StatsService(UdsClient& client)
        : client_(client) {}

    Stats fetch() {
        return client_.getStats();
    }

private:
    UdsClient& client_;
};

