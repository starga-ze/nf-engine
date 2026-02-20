#pragma once
#include <boost/beast.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <memory>

namespace beast = boost::beast;
namespace http = beast::http;
using tcp = boost::asio::ip::tcp;

class HttpRouter;

class HttpSession :
    public std::enable_shared_from_this<HttpSession>
{
public:
    HttpSession(tcp::socket socket,
                std::shared_ptr<HttpRouter> router);

    void run();

private:
    void doRead();
    void onRead(beast::error_code ec, std::size_t bytes);
    void onWrite(bool close,
                 beast::error_code ec,
                 std::size_t bytes);
    void doClose();

    tcp::socket m_socket;
    beast::flat_buffer m_buffer;
    http::request<http::string_body> m_req;
    std::shared_ptr<HttpRouter> m_router;
    std::shared_ptr<void> m_resHolder;
};

