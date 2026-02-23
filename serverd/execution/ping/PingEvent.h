#pragma once

#include "execution/Event.h"
#include "shard/ShardContext.h"

#include <vector>

class PingEvent : public ExternalEvent
{
protected:
    PingEvent(uint64_t sessionId);
};

class PingReqEvent : public PingEvent
{
public:
    PingReqEvent(uint64_t sessionId, std::vector<uint8_t> payload, 
            uint64_t nonce, uint64_t clientTs);

    void handleEvent(ShardContext& shardContext) override;
    uint64_t nonce() const;
    uint64_t clientTs() const;

private:
    std::vector<uint8_t> m_payload;
    uint64_t m_nonce;
    uint64_t m_clientTs;
};
