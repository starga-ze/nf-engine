#include "HttpSession.h"
#include "HttpRouter.h"

HttpSession::HttpSession(
    tcp::socket socket,
    std::shared_ptr<HttpRouter> router)
    : m_socket(std::move(socket)),
      m_router(std::move(router))
{
}

void HttpSession::run() 
{
    doRead();
}

void HttpSession::doRead() 
{
    m_req = {};
    http::async_read(m_socket, m_buffer, m_req, [self = shared_from_this()](auto ec, auto bytes)
    {
        self->onRead(ec, bytes);
    });
}

void HttpSession::onRead(
    beast::error_code ec,
    std::size_t)
{
    if (ec == http::error::end_of_stream)
        return doClose();
    if (ec)
        return;

    auto res = m_router->handle(m_req);
    bool close = res.need_eof();

    m_resHolder = std::make_shared<http::response<http::string_body>>(std::move(res));

    auto& resRef = *static_cast<http::response<http::string_body>*>(m_resHolder.get());

    http::async_write(m_socket, resRef, [self = shared_from_this(), close](auto ec, auto bytes)
    {
        self->onWrite(close, ec, bytes);
    });
}

void HttpSession::onWrite(bool close, beast::error_code ec, std::size_t)
{
    if (ec)
        return;

    if (close)
        return doClose();

    m_resHolder.reset();
    doRead();
}

void HttpSession::doClose() 
{
    beast::error_code ec;
    m_socket.shutdown(tcp::socket::shutdown_send, ec);
}

