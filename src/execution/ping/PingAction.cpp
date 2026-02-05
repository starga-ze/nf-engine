#include "execution/ping/PingAction.h"
#include "execution/ping/PingContext.h"
#include "execution/ping/PingBuilder.h"

PingAction::PingAction(uint64_t sessionId, Opcode opcode) :
    Action(sessionId, opcode)
{

}

PingResAction::PingResAction(uint64_t sessionId, Opcode opcode, uint64_t nonce, uint64_t clientTs) :
    PingAction(sessionId, opcode),
    m_nonce(nonce),
    m_clientTs(clientTs)
{

}

void PingResAction::handleAction(ShardContext& shardContext)
{
    m_payload = PingBuilder::serialize(opcode(), sessionId(), m_nonce, m_clientTs);
    shardContext.pingContext().pingResAction(*this);
}
