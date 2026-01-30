#include "LobbyContext.h"
#include "shard/ShardManager.h"
#include "egress/ActionFactory.h"
#include "util/Logger.h"

#include <cstring>
#include <string>

LobbyContext::LobbyContext(int shardIdx, ShardManager* shardManager) :
    m_shardManager(shardManager),
    m_shardIdx(shardIdx)
{
}

void LobbyContext::lobbyReqEvent(const LobbyReqEvent& ev) 
{
    const uint64_t sessionId = ev.sessionId();
    LOG_DEBUG("LOBBY_REQ received, [session={}]", sessionId);

    std::unique_ptr<Action> action = ActionFactory::create(Opcode::LOBBY_RES, sessionId);

    m_shardManager->commit(m_shardIdx, std::move(action));
}

void LobbyContext::setTxRouter(TxRouter *txRouter)
{
    m_txRouter = txRouter;
}

void LobbyContext::lobbyEntryAction(LobbyEntryAction& ac)
{
    if (not m_txRouter)
    {
        LOG_FATAL("TxRouter is nullptr");
        return;
    }

    const uint64_t sessionId = ac.sessionId();
    Opcode opcode = ac.opcode();

    LOG_DEBUG("LOBBY_ENTRY send, [session={}]", sessionId);

    auto payload = ac.takePayload();
    
    m_txRouter->handlePacket(sessionId, opcode, std::move(payload));
}
