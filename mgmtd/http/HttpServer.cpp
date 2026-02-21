#include "HttpServer.h"
#include "HttpListener.h"
#include "HttpRouter.h"
#include "HttpCache.h"

#include <boost/asio/ip/address.hpp>

HttpServer::HttpServer(int port,
                       std::shared_ptr<StatsService> svc,
                       std::shared_ptr<AuthService> authService)
    : m_port(port),
      m_statsService(std::move(svc)),
      m_authService(std::move(authService))
{
    m_ioc = std::make_shared<boost::asio::io_context>();

    auto cache = std::make_shared<HttpCache>(
        std::string(PROJECT_ROOT) + "/mgmtd/www");

    auto router = std::make_shared<HttpRouter>(m_statsService, cache, m_authService);

    auto endpoint = boost::asio::ip::tcp::endpoint(
            boost::asio::ip::make_address("0.0.0.0"), m_port);

    m_listener =
        std::make_shared<HttpListener>(*m_ioc, endpoint, router);

    m_listener->run();
}

void HttpServer::run() {
    m_ioc->run();
}

