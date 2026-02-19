#pragma once

#include "packet/ParsedPacketTypes.h"

class ShardContext;

class Action {
public:
    Action(uint64_t sessionId, Opcode opcode) : 
        m_sessionId(sessionId),
        m_opcode(opcode)
    {

    }
    virtual ~Action() = default;

    uint64_t sessionId() const { return m_sessionId; }

    Opcode opcode() const { return m_opcode; }

    virtual void handleAction(ShardContext &shardContext) = 0;

private:
    uint64_t m_sessionId;
    Opcode m_opcode;
};

