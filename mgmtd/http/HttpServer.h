#pragma once
#include <memory>
#include <boost/asio/io_context.hpp>

class StatsService;
class AuthService;
class HttpListener;

class HttpServer {
public:
    HttpServer(int port, std::shared_ptr<StatsService> svc, std::shared_ptr<AuthService> authService);
    void run();

private:
    int m_port;

    std::shared_ptr<StatsService> m_statsService;
    std::shared_ptr<AuthService> m_authService;
    std::shared_ptr<boost::asio::io_context> m_ioc;
    std::shared_ptr<HttpListener> m_listener;
};

