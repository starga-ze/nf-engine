#include "execution/lobby/LobbyEvent.h"
#include "execution/lobby/LobbyContext.h"

LobbyEvent::LobbyEvent(uint64_t sessionId):
    Event(sessionId)
{

}

LobbyEntryReqEvent::LobbyEntryReqEvent(uint64_t sessionId, std::vector<uint8_t> payload) :
    LobbyEvent(sessionId),
    m_payload(std::move(payload))
{

}

void LobbyEntryReqEvent::handleEvent(ShardContext& shardContext)
{
    shardContext.lobbyContext().lobbyEntryReqEvent(*this);
}
