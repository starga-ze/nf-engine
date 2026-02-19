#include "HttpServer.h"
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include <iostream>

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;
using json = nlohmann::json;

HttpServer::HttpServer(int port, StatsService& svc)
    : port_(port), svc_(svc) {}

void HttpServer::run() 
{
    boost::asio::io_context ioc{1};
    tcp::acceptor acceptor{ioc, {tcp::v4(), static_cast<unsigned short>(port_)}};

    while (true)
    {
    try
    {
        tcp::socket socket{ioc};
        acceptor.accept(socket);

        boost::beast::flat_buffer buffer;
        http::request<http::string_body> req;
        http::read(socket, buffer, req);

        http::response<http::string_body> res;
        res.version(req.version());
        res.set(http::field::server, "nf-mgmtd");
        res.set(http::field::content_type, "application/json");

        if (req.target() == "/api/v1/stats")
        {
            try
            {
                auto stats = svc_.fetch();

                json j{
                    {"rx_packets", stats.rx_packets},
                    {"tx_packets", stats.tx_packets},
                    {"active_sessions", stats.active_sessions}
                };

                res.result(http::status::ok);
                res.body() = j.dump();
            }
            catch (const std::exception& e)
            {
                res.result(http::status::internal_server_error);
                res.body() = std::string("{\"error\":\"") + e.what() + "\"}";
            }
        }
        else
        {
            res.result(http::status::not_found);
            res.body() = R"({"error":"not found"})";
        }

        res.prepare_payload();

        std::cout << res.body() << std::endl;

        http::write(socket, res);
    }
    catch (const std::exception& e)
    {
        std::cerr << "HTTP session error: " << e.what() << std::endl;
    }
    }
}

