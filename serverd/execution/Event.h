#pragma once

#include "shard/ShardContext.h"

#include <cstdint>

class Event 
{
    public:
        virtual ~Event() = default;
        virtual void handleEvent(ShardContext& shardContext) = 0;
};

class ExternalEvent : public Event
{
    public:
        explicit ExternalEvent(uint64_t sessionId)
            : m_sessionId(sessionId)
        {}

        uint64_t sessionId() const { return m_sessionId; }

    private:
        uint64_t m_sessionId;
};

class InternalEvent : public Event
{
    public:
        InternalEvent() = default;
};
