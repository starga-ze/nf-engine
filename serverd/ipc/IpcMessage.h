#pragma once

#include <cstdint>
#include <vector>
#include <string_view>

class IpcMessage
{
public:
    IpcMessage(int fd, std::vector<uint8_t>&& frame);

    int getFd() const;

    const std::vector<uint8_t>& getPayload() const;
    const uint8_t* data() const;
    size_t size() const;

    std::string_view body() const;

    uint16_t bodyLen() const;

    static std::vector<uint8_t> buildFrame(std::string_view body);

private:
    void validate();

private:
    static constexpr size_t HEADER_SIZE = 2;

    int m_fd{-1};
    std::vector<uint8_t> m_payload;
};
