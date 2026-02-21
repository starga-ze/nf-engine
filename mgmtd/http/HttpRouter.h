#pragma once

#include <memory>
#include <string>

#include <boost/beast/http.hpp>

class HttpCache;
class StatsService;
class AuthService;

namespace http = boost::beast::http;

class HttpRouter
{
public:
    using Request  = http::request<http::string_body>;
    using Response = http::response<http::string_body>;

    HttpRouter(std::shared_ptr<StatsService> statsService, std::shared_ptr<HttpCache> cache, 
            std::shared_ptr<AuthService> authService);

    Response handle(const Request& req);

private:
    Response handleStatic(const Request& req);
    Response handleLogin(const Request& req);
    Response dispatchProtected(const Request& req);

    bool isStatic(const std::string& target) const;
    std::string extractSession(const Request& req);

    Response makeResponse(http::status status, std::string body, std::string contentType, 
            unsigned version, bool keepAlive);

private:
    std::shared_ptr<StatsService> m_statsService;
    std::shared_ptr<HttpCache>    m_cache;
    std::shared_ptr<AuthService>  m_authService;
};
