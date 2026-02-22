#pragma once

#include "core/CoreControlTypes.h"

#include <cstdint>
#include <vector>
#include <string>

class CoreControl 
{
public:
    explicit CoreControl(class Core& core);

    EngineSnapshot engineSnapshot() const;
    SessionSnapshot sessionSnapshot() const;
    ShardSnapshot shardSnapshot() const; 
    MarketSnapshot marketSnapshot() const;

private:
    Core& m_core;
};
