#include "HttpServer.h"
#include "util/Logger.h"

#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <sstream>

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
    
            if (req.target() == "/")
            {
                LOG_INFO("HTTP req sent, project root: {}", std::string(PROJECT_ROOT));
                res.result(http::status::ok);
                res.set(http::field::content_type, "text/html");
                res.body() = loadFile(std::string(PROJECT_ROOT) + "/mgmtd/www/index.html");
            }
    
            else if (req.target() == "/app.js")
            {
                res.result(http::status::ok);
                res.set(http::field::content_type, "application/javascript");
                res.body() = loadFile(std::string(PROJECT_ROOT) + "/mgmtd/www/app.js");
            }

            else if (req.target() == "/style.css")
            {
                res.result(http::status::ok);
                res.set(http::field::content_type, "text/css");
                res.body() = loadFile(std::string(PROJECT_ROOT) + "/mgmtd/www/style.css");
            }
            else if (req.target() == "/api/v1/stats")
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


std::string HttpServer::loadFile(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        LOG_ERROR("Failed to open file: {}", path);
        throw std::runtime_error("Failed to open file: " + path);
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}
