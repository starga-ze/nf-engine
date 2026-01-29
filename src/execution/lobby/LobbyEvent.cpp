#include "execution/lobby/LobbyEvent.h"
#include "execution/lobby/LobbyContext.h"

LobbyEvent::LobbyEvent(uint64_t sessionId):
    Event(sessionId)
{

}

LobbyReqEvent::LobbyReqEvent(uint64_t sessionId, std::vector<uint8_t> payload) :
    LobbyEvent(sessionId),
    m_payload(std::move(payload))
{

}

void LobbyReqEvent::handleEvent(ShardContext& shardContext)
{
    shardContext.lobbyContext().lobbyReqEvent(*this);
}
