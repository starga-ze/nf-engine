#include "PingContext.h"
#include "shard/ShardManager.h"
#include "egress/ActionFactory.h"
#include "util/Logger.h"

#include <cstring>
#include <string>

PingContext::PingContext(int shardIdx, ShardManager* shardManager) :
    m_shardManager(shardManager),
    m_shardIdx(shardIdx)
{

}

void PingContext::pingReqEvent(const PingReqEvent& ev)
{
    const uint64_t sessionId = ev.sessionId();
    const uint64_t nonce = ev.nonce();
    const uint64_t clientTs = ev.clientTs();

    LOG_DEBUG("PING_REQ received, [session={}, nonce={}, clientTs={}]",
            sessionId, nonce, clientTs);

    auto action = std::make_unique<PingResAction>(sessionId, Opcode::PING_RES, nonce, clientTs);

    if (not action)
    {
        LOG_ERROR("PING_RES action create failed, [session={}]", sessionId);
        return;
    }

    m_shardManager->commit(m_shardIdx, std::move(action));
}

void PingContext::setTxRouter(TxRouter* txRouter)
{
    m_txRouter = txRouter;
}

void PingContext::pingResAction(PingResAction& ac)
{
    if (not m_txRouter)
    {
        LOG_FATAL("TxRouter is nullptr");
    }

    const uint64_t sessionId = ac.sessionId();
    Opcode opcode = ac.opcode();

    LOG_DEBUG("PING_RES send, [session={}]", sessionId);

    auto payload = ac.takePayload();

    m_txRouter->handlePacket(sessionId, opcode, std::move(payload));
}

