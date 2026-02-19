#include "HttpListener.h"
#include "HttpSession.h"

HttpListener::HttpListener(
    boost::asio::io_context& ioc,
    tcp::endpoint endpoint,
    std::shared_ptr<HttpRouter> router)
    : m_acceptor(ioc),
      m_router(std::move(router))
{
    m_acceptor.open(endpoint.protocol());
    m_acceptor.set_option(
        boost::asio::socket_base::reuse_address(true));
    m_acceptor.bind(endpoint);
    m_acceptor.listen();
}

void HttpListener::run() {
    doAccept();
}

void HttpListener::doAccept() {
    m_acceptor.async_accept(
        [self = shared_from_this()]
        (auto ec, auto socket)
        {
            if (!ec) {
                std::make_shared<HttpSession>(
                    std::move(socket),
                    self->m_router)->run();
            }
            self->doAccept();
        });
}

