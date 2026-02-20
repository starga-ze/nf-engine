#pragma once
#include <memory>
#include <boost/asio/io_context.hpp>

class StatsService;
class HttpListener;

class HttpServer {
public:
    HttpServer(int port, std::shared_ptr<StatsService> svc);
    void run();

private:
    int m_port;

    std::shared_ptr<StatsService> m_statsService;
    std::shared_ptr<boost::asio::io_context> m_ioc;
    std::shared_ptr<HttpListener> m_listener;
};

