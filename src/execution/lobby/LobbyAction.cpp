#include "execution/lobby/LobbyAction.h"
#include "execution/lobby/LobbyContext.h"
#include "execution/lobby/LobbyBuilder.h"

LobbyEntryAction::LobbyEntryAction(uint64_t sessionId, Opcode opcode) :
    m_sessionId(sessionId),
    m_opcode(opcode)
{
}

void LobbyEntryAction::handleAction(ShardContext& shardContext)
{
    m_payload = LobbyBuilder::serialize(m_opcode, m_sessionId, shardContext);
    shardContext.lobbyContext().lobbyEntryAction(*this);
}
