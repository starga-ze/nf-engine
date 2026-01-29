#include "LobbyContext.h"
#include "shard/ShardManager.h"

#include "execution/lobby/LobbyEvent.h"
#include "egress/ActionFactory.h"

#include "util/Logger.h"

#include <cstring>
#include <string>

LobbyContext::LobbyContext(int shardIdx, ShardManager* shardManager)
{
    m_shardManager = shardManager;
    m_shardIdx = shardIdx;
}

void LobbyContext::lobbyReqEvent(const LobbyReqEvent& ev) 
{
    const uint64_t sessionId = ev.sessionId();
    LOG_DEBUG("LOBBY_REQ received, [session={}]", sessionId);

    // std::unique_ptr<Action> action;
}

void LobbyContext::setTxRouter(TxRouter *txRouter)
{
    m_txRouter = txRouter;
}
