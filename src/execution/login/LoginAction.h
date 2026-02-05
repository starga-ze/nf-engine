#pragma once

#include "execution/Action.h"
#include "shard/ShardContext.h"

#include <cstdint>

class LoginAction : public Action 
{
protected:
    LoginAction(uint64_t sessionId, Opcode opcode);
};


class LoginSuccessResAction final : public LoginAction 
{
public:
    LoginSuccessResAction(uint64_t sessionId, Opcode opcode);

    void handleAction(ShardContext &shardContext) override;

    const std::vector<uint8_t> takePayload() { return std::move(m_payload); }

private:
    std::vector<uint8_t> m_payload;
};


class LoginFailResAction final : public LoginAction 
{
public:
    LoginFailResAction(uint64_t sessionId, Opcode opcode);

    void handleAction(ShardContext &shardContext) override;

    const std::vector<uint8_t> takePayload() { return std::move(m_payload); }

private:
    std::vector<uint8_t> m_payload;
};



