#include "execution/ping/PingAction.h"
#include "execution/ping/PingContext.h"
#include "execution/ping/PingBuilder.h"

PingResAction::PingResAction(uint64_t sessionId, Opcode opcode, uint64_t nonce, uint64_t clientTs) :
    m_sessionId(sessionId),
    m_opcode(opcode),
    m_nonce(nonce),
    m_clientTs(clientTs)
{

}

void PingResAction::handleAction(ShardContext& shardContext)
{
    m_payload = PingBuilder::serialize(m_opcode, m_sessionId, m_nonce, m_clientTs);
    shardContext.pingContext().pingResAction(*this);
}
