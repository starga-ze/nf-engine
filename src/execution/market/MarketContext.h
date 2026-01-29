#pragma once

#include <cstdint>
#include <unordered_map>
#include <memory>


class MarketContext {
public:
    explicit MarketContext(int shardIdx);
    ~MarketContext();

    void tick(uint64_t deltaMs);

private:
    void init();

    int m_shardIdx;
};

