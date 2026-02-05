#pragma once

#include "execution/Action.h"
#include "shard/ShardContext.h"

#include <cstdint>

class PingAction : public Action
{
protected:
    PingAction(uint64_t sessionId, Opcode opcode);
};

class PingResAction final : public PingAction
{
public:
    PingResAction(uint64_t sessionId, Opcode opcode, uint64_t nonce, uint64_t clientTs);

    void handleAction(ShardContext& shardContext) override;
    const std::vector<uint8_t> takePayload() { return std::move(m_payload); }

private:
    std::vector<uint8_t> m_payload;
    uint64_t m_nonce;
    uint64_t m_clientTs;
};
