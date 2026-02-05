#include "execution/lobby/LobbyAction.h"
#include "execution/lobby/LobbyContext.h"
#include "execution/lobby/LobbyBuilder.h"

LobbyAction::LobbyAction(uint64_t sessionId, Opcode opcode) :
    Action(sessionId, opcode)
{

}

LobbyEntryResAction::LobbyEntryResAction(uint64_t sessionId, Opcode opcode) :
    LobbyAction(sessionId, opcode)
{
}

void LobbyEntryResAction::handleAction(ShardContext& shardContext)
{
    m_payload = LobbyBuilder::serialize(opcode(), sessionId(), shardContext);
    shardContext.lobbyContext().lobbyEntryResAction(*this);
}
