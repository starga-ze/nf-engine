#include "IpcCommandHandler.h"
#include "core/CoreControl.h"

using json = nlohmann::json;

IpcCommandHandler::IpcCommandHandler(CoreControl* control)
    : m_control(control)
{
}

std::string IpcCommandHandler::handle(const std::string& request)
{
    try
    {
        auto req = json::parse(request);
        std::string cmd = req.value("cmd", "");

        if (cmd == "stats/session")
            return handleSessionStats();

        if (cmd == "stats/engine")
            return handleEngineStats();

        if (cmd == "stats/shard")
            return handleShardStats();

        if (cmd == "stats/market")
            return handleMarketStats();

        return R"({"error":"unknown command"})";
    }
    catch (...)
    {
        return R"({"error":"invalid json"})";
    }
}

std::string IpcCommandHandler::handleEngineStats()
{
    auto snapshot = m_control->engineSnapshot();

    json res;

    res["ok"]           = true;
    res["uptime"]       = snapshot.uptimeSeconds;
    res["rssMB"]        = snapshot.rssMB;
    res["heapMB"]       = snapshot.heapMB;
    res["dataMB"]       = snapshot.dataMB;
    res["virtualMB"]    = snapshot.virtualMB;
    res["cpuPercent"]   = snapshot.cpuPercent;

    return res.dump();
}

std::string IpcCommandHandler::handleSessionStats()
{
    auto snapshot = m_control->sessionSnapshot();

    json res;

    res["totalSessions"] = snapshot.totalSessions;

    json arr = json::array();

    for (const auto& s : snapshot.sessions)
    {
        arr.push_back({
                {"sid", s.sessionId},
                {"state", s.state},
                {"tls_fd", s.tlsFd},
                {"tcp_fd", s.tcpFd},
                {"udp_fd", s.udpFd}
                });
    }

    res["sessions"] = arr;

    return res.dump();
}

std::string IpcCommandHandler::handleShardStats()
{
    auto snapshot = m_control->shardSnapshot();

    json res;

    res["shardCount"] = snapshot.shardCount;

    return res.dump();
}

std::string IpcCommandHandler::handleMarketStats()
{
    auto snapshot = m_control->marketSnapshot();

    json res;
    json marketsArr = json::array();

    for (const auto& market : snapshot.markets)
    {
        json marketJson;
        marketJson["marketId"] = market.marketId;
        marketJson["itemCount"] = market.items.size();

        json itemsArr = json::array();

        for (const auto& item : market.items)
        {
            json itemJson;
            itemJson["item_id"] = item.itemId;
            itemJson["seller_session_id"] = item.sellerSessionId;
            itemJson["name"] = item.name;
            itemJson["price"] = item.price;
            itemJson["quantity"] = item.quantity;
            itemJson["created_at_sec"] = item.createdAt;

            itemsArr.push_back(std::move(itemJson));
        }

        marketJson["items"] = std::move(itemsArr);

        marketsArr.push_back(std::move(marketJson));
    }

    res["markets"] = std::move(marketsArr);

    return res.dump();
}
