#include "MarketContext.h"
#include "util/Logger.h"

#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;

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
    LOG_TRACE("MarketContext initialzie, shard idx: {}, market id:{}", m_shardIdx, m_marketId);

    m_snapshotPath = std::string(PROJECT_ROOT)
        + "/database/serverd-market-"
        + std::to_string(m_marketId)
        + ".json";

    loadSnapshot();
}

/* Internal Event, future model is no need for action enqueue */
void MarketContext::marketDumpReqEvent(MarketDumpReqEvent& ev)
{
    auto snap = snapshot();
    ev.fulfill(std::move(snap));
}

void MarketContext::loadSnapshot()
{
    std::ifstream ifs(m_snapshotPath);
    if (!ifs.is_open())
    {
        LOG_WARN("Market snapshot not found: {}, start empty market", m_snapshotPath);
        m_nextItemId = 1;
        return;
    }

    json j;

    try
    {
        ifs >> j;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Market snapshot parse error: {}, path:{}", e.what(), m_snapshotPath);
        m_nextItemId = 1;
        return;
    }

    m_items.clear();

    uint32_t version = j.value("version", 1U);

    m_nextItemId = j.value("next_item_id", 1ULL);

    if (!j.contains("items") || !j["items"].is_array())
    {
        LOG_WARN("Market snapshot has no valid items array: {}",
                m_snapshotPath);
        return;
    }

    for (const auto& item : j["items"])
    {
        if (!item.is_object())
            continue;

        MarketItem mi;

        mi.itemId          = item.value("item_id", 0ULL);
        mi.sellerSessionId = item.value("seller_session_id", 0ULL);
        mi.name            = item.value("name", "");
        mi.price           = item.value("price", 0ULL);
        mi.quantity        = item.value("quantity", 0U);
        mi.createdAt       = item.value("created_at_sec", 0ULL);

        if (mi.itemId == 0)
        {
            LOG_WARN("Invalid item_id in snapshot, skip");
            continue;
        }

        m_items.emplace(mi.itemId, std::move(mi));
    }

    LOG_INFO("Market snapshot loaded, marketId:{}, version:{}, itemCount:{}, nextItemId:{}",
            m_marketId,
            version,
            m_items.size(),
            m_nextItemId);
}

MarketInfo MarketContext::snapshot() const
{
    MarketInfo marketInfo;
    marketInfo.marketId = m_marketId;

    marketInfo.items.reserve(m_items.size());

    for (const auto& [id, item] : m_items)
    {
        MarketItem mi;
        mi.itemId          = item.itemId;
        mi.sellerSessionId = item.sellerSessionId;
        mi.name            = item.name;
        mi.price           = item.price;
        mi.quantity        = item.quantity;
        mi.createdAt       = item.createdAt;

        marketInfo.items.push_back(std::move(mi));
    }

    return marketInfo;
}

void MarketContext::tick() {

}

