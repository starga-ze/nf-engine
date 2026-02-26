#include "ipc/IpcMessage.h"

#include <stdexcept>
#include <cstring>

IpcMessage::IpcMessage(int fd, std::vector<uint8_t>&& frame)
    : m_fd(fd),
      m_payload(std::move(frame))
{
    validate();
}

int IpcMessage::getFd() const
{
    return m_fd;
}

const std::vector<uint8_t>& IpcMessage::getPayload() const
{
    return m_payload;
}

const uint8_t* IpcMessage::data() const
{
    return m_payload.data();
}

size_t IpcMessage::size() const
{
    return m_payload.size();
}

std::string_view IpcMessage::body() const
{
    if (m_payload.size() <= HEADER_SIZE)
        return {};

    return std::string_view(
        reinterpret_cast<const char*>(m_payload.data() + HEADER_SIZE),
        m_payload.size() - HEADER_SIZE);
}

uint16_t IpcMessage::bodyLen() const
{
    if (m_payload.size() < HEADER_SIZE)
        return 0;

    return (static_cast<uint16_t>(m_payload[0]) << 8) |
           static_cast<uint16_t>(m_payload[1]);
}

void IpcMessage::validate()
{
    if (m_payload.size() < HEADER_SIZE)
        throw std::runtime_error("IpcMessage: invalid frame");

    const uint16_t len = bodyLen();

    if (m_payload.size() != static_cast<size_t>(len) + HEADER_SIZE)
        throw std::runtime_error("IpcMessage: length mismatch");
}

std::vector<uint8_t> IpcMessage::buildFrame(std::string_view body)
{
    if (body.size() > 0xFFFF)
        body = body.substr(0, 0xFFFF);

    const uint16_t len = static_cast<uint16_t>(body.size());

    std::vector<uint8_t> frame(len + HEADER_SIZE);

    frame[0] = static_cast<uint8_t>((len >> 8) & 0xFF);
    frame[1] = static_cast<uint8_t>(len & 0xFF);

    if (len > 0)
        std::memcpy(frame.data() + HEADER_SIZE, body.data(), len);

    return frame;
}
