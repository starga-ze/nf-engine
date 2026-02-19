#include "UdsClient.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

UdsClient::UdsClient(std::string path)
    : socketPath_(std::move(path)) {}

Stats UdsClient::getStats() {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0)
        throw std::runtime_error("socket() failed");

    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, socketPath_.c_str(),
                 sizeof(addr.sun_path) - 1);

    if (connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        throw std::runtime_error("connect() failed");
    }

    const char* req = R"({"cmd":"stats"})";
    if (write(fd, req, strlen(req)) < 0) {
        close(fd);
        throw std::runtime_error("write() failed");
    }

    char buffer[4096];
    int n = read(fd, buffer, sizeof(buffer));
    close(fd);

    if (n <= 0)
        throw std::runtime_error("read() failed");

    std::string response(buffer, n);

    std::cout << "RAW RESPONSE: " << response << std::endl;

    json j = json::parse(response);

    Stats stats{
        j["rx_packets"],
        j["tx_packets"],
        j["active_sessions"]
    };

    std::cout
        << "rx=" << stats.rx_packets
        << " tx=" << stats.tx_packets
        << " active=" << stats.active_sessions
        << std::endl;

    return stats;
}

