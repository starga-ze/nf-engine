#include "HttpRouter.h"
#include "HttpCache.h"
#include "service/StatsService.h"
#include "util/Logger.h"

#include <string>

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
    http::response<http::string_body> res{st, version};
    res.set(http::field::server, "nf-mgmtd");
    res.set(http::field::content_type, type);
    res.keep_alive(keepAlive);
    res.body() = std::move(body);
    res.prepare_payload();
    return res;
}

http::response<http::string_body>
HttpRouter::handle(const http::request<http::string_body>& req)
{
    const std::string target = std::string(req.target());
    const bool keep = req.keep_alive();

    // Static files
    if (target == "/" ||
        target.rfind("/js/", 0) == 0 ||
        target.rfind("/css/", 0) == 0)
    {
        auto file = m_cache->get(target);
        if (!file)
        {
            return makeResponse(http::status::not_found,
                                "not found",
                                "text/plain",
                                req.version(),
                                keep);
        }

        return makeResponse(http::status::ok,
                            file->body,
                            file->contentType,
                            req.version(),
                            keep);
    }

    // API - session stats
    if (target == "/api/v1/stats/session")
    {
        const std::string jsonBody = m_statsService->fetchSession();
        
        LOG_TRACE("HTTP /api/v1/stats/session response: {}", jsonBody);

        return makeResponse(http::status::ok,
                            jsonBody,
                            "application/json",
                            req.version(),
                            keep);
    }

    // API - engine stats
    if (target == "/api/v1/stats/engine")
    {
        const std::string jsonBody = m_statsService->fetchEngine();

        LOG_TRACE("HTTP /api/v1/stats/engine response: {}", jsonBody);

        return makeResponse(http::status::ok,
                            jsonBody,
                            "application/json",
                            req.version(),
                            keep);
    }

    return makeResponse(http::status::not_found,
                        R"({"error":"not found"})",
                        "application/json",
                        req.version(),
                        keep);
}
