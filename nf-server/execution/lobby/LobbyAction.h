#pragma once

#include "execution/Action.h"
#include "shard/ShardContext.h"

#include <cstdint>

class LobbyAction : public Action 
{
protected:
    LobbyAction(uint64_t sessionId, Opcode opcode);
};

class LobbyEntryResAction final : public LobbyAction
{
public:
    LobbyEntryResAction(uint64_t sessionId, Opcode opcode);

    void handleAction(ShardContext& shardContext) override;

    const std::vector<uint8_t> takePayload() { return std::move(m_payload); }

private:
    std::vector<uint8_t> m_payload;
};
