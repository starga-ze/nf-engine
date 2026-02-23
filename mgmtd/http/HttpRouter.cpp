#include "HttpRouter.h"

#include "HttpCache.h"
#include "service/StatsService.h"
#include "service/AuthService.h"

#include "util/Logger.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace http = boost::beast::http;

HttpRouter::HttpRouter(std::shared_ptr<StatsService> statsService,
        std::shared_ptr<HttpCache> cache,
        std::shared_ptr<AuthService> authService)
    : m_statsService(std::move(statsService)),
    m_cache(std::move(cache)),
    m_authService(std::move(authService))
{
}

HttpRouter::Response HttpRouter::handle(const Request& req)
{
    const std::string target = std::string(req.target());
    const bool keep = req.keep_alive();

    LOG_TRACE("HTTP reqeust arrive, target {}", target);

    if (isStatic(target))
        return handleStatic(req);

    if (target == "/api/login" && req.method() == http::verb::post)
    {
        return handleLogin(req);
    }

    if (target == "/api/logout" && req.method() == http::verb::post)
    {
        auto sessionId = extractSession(req);

        LOG_TRACE("logout sessionId: [{}]", sessionId);
        m_authService->logout(sessionId);

        Response res = makeResponse(
                http::status::ok,
                R"({"status":"logged_out"})",
                "application/json",
                req.version(),
                keep);

        res.set(http::field::set_cookie,
                "session=; Path=/; Max-Age=0");

        return res;
    }

    if (target.rfind("/api/v1/", 0) == 0)
    {
        std::string sessionId = extractSession(req);

        if (!m_authService->validateSession(sessionId))
        {
            return makeResponse(
                    http::status::unauthorized,
                    R"({"error":"unauthorized"})",
                    "application/json",
                    req.version(),
                    keep);
        }

        return dispatchProtected(req);
    }

    return makeResponse(
            http::status::not_found,
            R"({"error":"not found"})",
            "application/json",
            req.version(),
            keep);
}

bool HttpRouter::isStatic(const std::string& target) const
{
    return target == "/" ||
        target.rfind("/js/", 0) == 0 ||
        target.rfind("/css/", 0) == 0 ||
        target.rfind("/img/", 0) == 0;
}

HttpRouter::Response HttpRouter::handleStatic(const Request& req)
{
    const bool keep = req.keep_alive();
    const std::string target = std::string(req.target());

    auto file = m_cache->get(target);
    if (!file)
    {
        return makeResponse(
                http::status::not_found,
                "not found",
                "text/plain",
                req.version(),
                keep);
    }

    return makeResponse(
            http::status::ok,
            file->body,
            file->contentType,
            req.version(),
            keep);
}

HttpRouter::Response HttpRouter::handleLogin(const Request& req)
{
    const bool keep = req.keep_alive();

    try
    {
        auto body = json::parse(req.body());

        std::string username = body.at("username");
        std::string password = body.at("password");

        auto result = m_authService->login(username, password);

        if (!result.success)
        {
            return makeResponse(
                    http::status::unauthorized,
                    R"({"error":"invalid credentials"})",
                    "application/json",
                    req.version(),
                    keep);
        }

        Response res = makeResponse(
                http::status::ok,
                R"({"status":"ok"})",
                "application/json",
                req.version(),
                keep);

        res.set(http::field::set_cookie,
                "session=" + result.sessionId +
                "; HttpOnly; Path=/; SameSite=Strict");

        return res;
    }
    catch (...)
    {
        return makeResponse(
                http::status::bad_request,
                R"({"error":"bad request"})",
                "application/json",
                req.version(),
                keep);
    }
}

HttpRouter::Response HttpRouter::dispatchProtected(const Request& req)
{
    const std::string target = std::string(req.target());
    const bool keep = req.keep_alive();

    std::string jsonBody;

    if (target == "/api/v1/stats/session")
    {
        jsonBody = m_statsService->fetchSession();
    }
    else if (target == "/api/v1/stats/engine")
    {
        jsonBody = m_statsService->fetchEngine();
    }
    else if (target == "/api/v1/stats/shard")
    {
        jsonBody = m_statsService->fetchShard();
    }
    else if (target == "/api/v1/stats/market")
    {
        jsonBody = m_statsService->fetchMarket();
    }
    else
    {
        LOG_TRACE("Response dump: {} -> NOT_FOUND", target);

        return makeResponse(
                http::status::not_found,
                R"({"error":"not found"})",
                "application/json",
                req.version(),
                keep);
    }

    LOG_TRACE("Response dump: {} -> {}", target, jsonBody);

    return makeResponse(
            http::status::ok,
            std::move(jsonBody),
            "application/json",
            req.version(),
            keep);
}

std::string HttpRouter::extractSession(const Request& req)
{
    auto it = req.find(http::field::cookie);
    if (it == req.end())
        return "";

    std::string cookies(it->value().data(), it->value().size());

    const std::string key = "session=";
    auto pos = cookies.find(key);
    if (pos == std::string::npos)
        return "";

    auto end = cookies.find(";", pos);
    if (end == std::string::npos)
        end = cookies.size();

    return cookies.substr(pos + key.size(),
            end - (pos + key.size()));
}

HttpRouter::Response HttpRouter::makeResponse(http::status status,
        std::string body,
        std::string contentType,
        unsigned version,
        bool keepAlive)
{
    Response res{status, version};

    res.set(http::field::server, "nf-mgmtd");
    res.set(http::field::content_type, contentType);
    res.keep_alive(keepAlive);

    res.body() = std::move(body);
    res.prepare_payload();

    return res;
}
