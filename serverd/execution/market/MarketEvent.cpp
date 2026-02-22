#include "execution/market/MarketEvent.h"
#include "execution/market/MarketContext.h"

#include "util/Logger.h"

MarketDumpReqEvent::MarketDumpReqEvent(std::promise<MarketInfo> marketInfo)
    : m_marketInfo(std::move(marketInfo))
{
}

void MarketDumpReqEvent::handleEvent(ShardContext& shardContext)
{
    LOG_TRACE("market dump req event handle event");
    shardContext.marketContext().marketDumpReqEvent(*this);
}

void MarketDumpReqEvent::fulfill(MarketInfo marketInfo)
{
    m_marketInfo.set_value(std::move(marketInfo));
}
