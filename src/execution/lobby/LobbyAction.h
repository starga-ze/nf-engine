#pragma once

#include "execution/Action.h"
#include "shard/ShardContext.h"

#include <cstdint>

class LobbyAction : public Action 
{
public:
    LobbyAction() = default;
    virtual ~LobbyAction() = default;
};

class LobbyEntryResAction final : public LobbyAction
{
public:
    LobbyEntryResAction(uint64_t sessionId, Opcode opcode);

    void handleAction(ShardContext& shardContext) override;

    const std::vector<uint8_t> takePayload() { return std::move(m_payload); }

    uint64_t sessionId() const { return m_sessionId; }

    Opcode opcode() const { return m_opcode; }

private:
    Opcode m_opcode;
    uint64_t m_sessionId;
    std::vector<uint8_t> m_payload;
};
