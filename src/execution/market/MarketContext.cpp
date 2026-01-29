#include "MarketContext.h"
#include "util/Logger.h"

MarketContext::MarketContext(int shardIdx, uint8_t marketId) : 
    m_shardIdx(shardIdx),
    m_marketId(marketId)
{
    init();
}

MarketContext::~MarketContext() {
    
}

void MarketContext::init() 
{
    LOG_TRACE("MarketContext initialize, shard idx:{}, market id:{}", m_shardIdx, m_marketId);
    //TODO, load snapshot with database (only 1st check)
}

void MarketContext::tick() {

}

