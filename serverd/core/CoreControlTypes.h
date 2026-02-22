#pragma once

#include <cstdint>
#include <vector>
#include <string>

// Engine
struct EngineSnapshot
{
    uint64_t uptimeSeconds;
    uint64_t rssMB;
    uint64_t heapMB;
    uint64_t dataMB;
    uint64_t virtualMB;
    double cpuPercent;
};

// Session
struct SessionInfo
{
    uint64_t sessionId;
    std::string state;
    int tlsFd;
    int tcpFd;
    int udpFd;
};

struct SessionSnapshot
{
    uint64_t totalSessions = 0;
    std::vector<SessionInfo> sessions;
};

// Shard
struct ShardSnapshot
{
    size_t shardCount;
};

// Market
struct MarketItem
{
    uint64_t itemId;
    uint64_t sellerSessionId;
    std::string name;
    uint64_t price;
    uint32_t quantity;
    uint64_t createdAt;
};

struct MarketInfo
{
    uint8_t marketId;
    std::vector<MarketItem> items;
};

struct MarketSnapshot
{
    std::vector<MarketInfo> markets;
};
