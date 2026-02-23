#pragma once

#include "execution/market/MarketEvent.h"
#include "core/CoreControlTypes.h"

#include <cstdint>
#include <unordered_map>
#include <memory>
#include <string>

class MarketContext {
public:
    explicit MarketContext(int shardIdx, uint8_t marketId);
    ~MarketContext();

    void tick();
    void marketDumpReqEvent(MarketDumpReqEvent& ev);

private:
    void init();
    void loadSnapshot();
    MarketInfo snapshot() const;

    int m_shardIdx;
    uint8_t m_marketId;

    uint64_t m_nextItemId {1};

    std::unordered_map<uint64_t, MarketItem> m_items;

    std::string m_snapshotPath;
};

