#pragma once
#include <string>
#include <cstdint>

struct Stats {
    uint64_t rx_packets{};
    uint64_t tx_packets{};
    uint64_t active_sessions{};
};

class UdsClient {
public:
    explicit UdsClient(std::string path);
    Stats getStats();

private:
    std::string socketPath_;
};

