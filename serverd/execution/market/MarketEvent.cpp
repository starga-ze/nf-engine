#include "execution/market/MarketEvent.h"
#include "execution/market/MarketContext.h"

MarketEvent::MarketEvent(uint64_t sessionId) :
    ExternalEvent(sessionId)
{

}

MarketDumpReqEvent::MarketDumpReqEvent(std::promise<MarketInfo> marketInfo)
    : m_marketInfo(std::move(marketInfo))
{
}

void MarketDumpReqEvent::handleEvent(ShardContext& shardContext)
{
    shardContext.marketContext().marketDumpReqEvent(*this);
}

void MarketDumpReqEvent::fulfill(MarketInfo marketInfo)
{
    m_marketInfo.set_value(std::move(marketInfo));
}
