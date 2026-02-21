#include "HttpSession.h"
#include "HttpRouter.h"

HttpSession::HttpSession(tcp::socket socket, std::shared_ptr<HttpRouter> router) : 
    m_socket(std::move(socket)),
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

    http::async_read(m_socket, m_buffer, m_req, 
            beast::bind_front_handler(&HttpSession::onRead, shared_from_this()));
}

void HttpSession::onRead(beast::error_code ec, std::size_t)
{
    if (ec == http::error::end_of_stream)
        return doClose();
    if (ec)
        return;

    auto res = m_router->handle(m_req);
    bool close = res.need_eof();

    m_resHolder = std::make_shared<http::response<http::string_body>>(std::move(res));

    auto& resRef = *static_cast<http::response<http::string_body>*>(m_resHolder.get());
    
    http::async_write(m_socket, resRef, 
            beast::bind_front_handler(&HttpSession::onWrite, shared_from_this(), close));
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

