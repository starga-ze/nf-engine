#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <cstring>

class IpcMessage
{
public:
    explicit IpcMessage(int fd, std::string body)
        : m_fd(fd)
    {
        buildPayload(std::move(body));
    }

    int getFd() const { return m_fd; }
    const std::vector<uint8_t>& getPayload() const { return m_payload; }
    size_t size() const { return m_payload.size(); }

private:
    void buildPayload(std::string body)
    {
        if (body.size() > 0xFFFF)
        {
            body.resize(0xFFFF);
        }

        const uint16_t len = static_cast<uint16_t>(body.size());

        m_payload.resize(static_cast<size_t>(len) + 2);
        m_payload[0] = static_cast<uint8_t>((len >> 8) & 0xFF);
        m_payload[1] = static_cast<uint8_t>(len & 0xFF);

        if (len > 0)
        {
            std::memcpy(m_payload.data() + 2, body.data(), len);
        }
    }

    int m_fd{-1};
    std::vector<uint8_t> m_payload;
};
