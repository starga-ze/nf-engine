#pragma once

#include <cstdint>

class ShardContext;

class Event {
public:
    Event() = default;
    virtual ~Event() = default;

    virtual void handleEvent(ShardContext &shardContext) = 0;
};

