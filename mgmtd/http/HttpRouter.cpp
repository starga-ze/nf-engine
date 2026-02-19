#include "HttpRouter.h"
#include "HttpCache.h"
#include "service/StatsService.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

HttpRouter::HttpRouter(
    std::shared_ptr<StatsService> svc,
    std::shared_ptr<HttpCache> cache)
    : m_statsService(std::move(svc)),
      m_cache(std::move(cache))
{
}

static http::response<http::string_body>
makeResponse(http::status st,
             std::string body,
             std::string type,
             unsigned version,
             bool keepAlive)
{
    http::response<http::string_body>
        res{st, version};

    res.set(http::field::server, "nf-mgmtd");
    res.set(http::field::content_type, type);
    res.keep_alive(keepAlive);
    res.body() = std::move(body);
    res.prepare_payload();
    return res;
}

http::response<http::string_body>
HttpRouter::handle(
    const http::request<
        http::string_body>& req)
{
    const std::string target =
        std::string(req.target());
    const bool keep =
        req.keep_alive();

    if (target == "/"
        || target == "/app.js"
        || target == "/style.css")
    {
        auto file =
            m_cache->get(target);

        if (!file)
            return makeResponse(
                http::status::not_found,
                "not found",
                "text/plain",
                req.version(),
                keep);

        return makeResponse(
            http::status::ok,
            file->body,
            file->contentType,
            req.version(),
            keep);
    }

    if (target == "/api/v1/stats")
    {
        auto stats = m_statsService->fetch();

        json j;
        j["rx_packets"] = stats.rx_packets;
        j["tx_packets"] = stats.tx_packets;
        j["active_sessions"] = stats.active_sessions;

        return makeResponse(
            http::status::ok,
            j.dump(),
            "application/json",
            req.version(),
            keep);
    }

    return makeResponse(
        http::status::not_found,
        R"({"error":"not found"})",
        "application/json",
        req.version(),
        keep);
}

