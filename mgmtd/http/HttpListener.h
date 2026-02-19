#pragma once
#include <boost/asio/ip/tcp.hpp>
#include <memory>

using tcp = boost::asio::ip::tcp;

class HttpRouter;

class HttpListener :
    public std::enable_shared_from_this<HttpListener>
{
public:
    HttpListener(boost::asio::io_context& ioc,
                 tcp::endpoint endpoint,
                 std::shared_ptr<HttpRouter> router);

    void run();

private:
    void doAccept();

    tcp::acceptor m_acceptor;
    std::shared_ptr<HttpRouter> m_router;
};

