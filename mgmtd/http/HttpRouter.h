#pragma once
#include <memory>
#include <boost/beast/http.hpp>

namespace http = boost::beast::http;

class StatsService;
class HttpCache;

class HttpRouter {
public:
    HttpRouter(std::shared_ptr<StatsService> svc,
               std::shared_ptr<HttpCache> cache);

    http::response<http::string_body>
    handle(const http::request<http::string_body>& req);

private:
    std::shared_ptr<StatsService> m_statsService;
    std::shared_ptr<HttpCache> m_cache;
};

