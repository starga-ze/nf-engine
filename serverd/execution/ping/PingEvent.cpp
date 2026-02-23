#include "execution/ping/PingEvent.h"
#include "execution/ping/PingContext.h"

PingEvent::PingEvent(uint64_t sessionId) :
    ExternalEvent(sessionId)
{

}

PingReqEvent::PingReqEvent(uint64_t sessionId, std::vector<uint8_t> payload, 
        uint64_t nonce, uint64_t clientTs) :
    PingEvent(sessionId),
    m_payload(std::move(payload)),
    m_nonce(nonce),
    m_clientTs(clientTs)
{

}

void PingReqEvent::handleEvent(ShardContext& shardContext)
{
    shardContext.pingContext().pingReqEvent(*this);
}

uint64_t PingReqEvent::nonce() const
{
    return m_nonce;
}

uint64_t PingReqEvent::clientTs() const
{
    return m_clientTs;
}
