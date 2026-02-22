#pragma once

#include "execution/Event.h"

#include <vector>

class LobbyEvent : public ExternalEvent
{
protected:
    LobbyEvent(uint64_t sessionId);
};

class LobbyEntryReqEvent : public LobbyEvent
{
public:
    LobbyEntryReqEvent(uint64_t sessionId, std::vector<uint8_t> payload);

    void handleEvent(ShardContext& shardContext) override;

private:
    std::vector<uint8_t> m_payload;
};
